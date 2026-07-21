#include "Utilities.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>

bool Utilities::RunProcess(const QString& program, const QStringList& args, QByteArray* output)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        return false;
    }

    if (output != nullptr)
    {
        *output = process.readAllStandardOutput();
    }

    return true;
}

bool Utilities::HttpGet(const QString& url, QByteArray* output)
{
    static QNetworkAccessManager manager;

    QUrl currentUrl = QUrl(url);
    for (int redirectsRemaining = 5; redirectsRemaining > 0; --redirectsRemaining)
    {
        QNetworkRequest request(currentUrl);
        request.setRawHeader("User-Agent", "NickelUpdater");
        request.setRawHeader("Accept", "application/vnd.github+json");

        QScopedPointer<QNetworkReply> reply(manager.get(request));
        QEventLoop loop;
        QObject::connect(reply.data(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError)
        {
            nh_log("HTTP GET failed for %s: %s", qPrintable(currentUrl.toString()), qPrintable(reply->errorString()));
            return false;
        }

        const auto redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirectTarget.isEmpty())
        {
            const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode < 200 || statusCode >= 300)
            {
                nh_log("HTTP GET returned status %d for %s", statusCode, qPrintable(currentUrl.toString()));
                return false;
            }

            if (output != nullptr)
            {
                *output = reply->readAll();
            }

            return true;
        }

        currentUrl = currentUrl.resolved(redirectTarget);
    }

    nh_log("Too many redirects for %s", qPrintable(url));
    return false;
}

bool Utilities::DownloadFile(const QString& url, const QString& outputPath)
{
    QByteArray output;
    if (!HttpGet(url, &output))
    {
        return false;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        nh_log("Failed to open %s for writing", qPrintable(outputPath));
        return false;
    }

    return file.write(output) == output.size();
}

QString Utilities::StageDirectoryForPlugin(const QString& pluginId)
{
    return QDir(STAGING_DIR).filePath(pluginId);
}

QString Utilities::MergedArchivePath()
{
    return QDir(STAGING_DIR).filePath("KoboRoot.merged.tgz");
}

bool Utilities::ExtractArchive(const QString& archivePath, const QString& outputDir)
{
    return RunProcess("tar", {"-xzf", archivePath, "-C", outputDir});
}

bool Utilities::CreateArchive(const QString& sourceDir, const QString& archivePath)
{
    QFile::remove(archivePath);
    return RunProcess("tar", {"-czf", archivePath, "-C", sourceDir, "."});
}

bool Utilities::PublishArchive(const QString& archivePath)
{
    QFile::remove(KOBOROOT_PATH);
    return QFile::copy(archivePath, KOBOROOT_PATH);
}

bool Utilities::RebootDevice()
{
    return QProcess::startDetached("reboot");
}

bool Utilities::PrepareMergeDirectory(const QString& mergeDirPath)
{
    QDir stagingRoot(STAGING_DIR);
    if (stagingRoot.exists() && !stagingRoot.removeRecursively())
    {
        nh_log("Failed to clear staging directory: %s", STAGING_DIR);
        return false;
    }

    if (!QDir().mkpath(mergeDirPath))
    {
        nh_log("Failed to create merge directory: %s", qPrintable(mergeDirPath));
        return false;
    }

    return true;
}

QString Utilities::ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath)
{
    const auto release = GitHubInterface::GetLatestRelease(plugin.PluginId);
    if (!release.IsValid())
    {
        nh_log("Failed to load latest release for %s", qPrintable(plugin.PluginId));
        return {};
    }

    if (!plugin.TagName.isEmpty() && plugin.TagName == release.TagName)
    {
        nh_log("Plugin %s already at %s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));
        return {};
    }

    const auto stageDirPath = StageDirectoryForPlugin(plugin.PluginId);
    if (!QDir().mkpath(stageDirPath))
    {
        nh_log("Failed to create stage dir for %s", qPrintable(plugin.PluginId));
        return {};
    }

    const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
    if (!DownloadFile(release.KoboRootUrl, stageFilePath))
    {
        nh_log("Failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    if (!ExtractArchive(stageFilePath, mergeDirPath))
    {
        nh_log("Failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    nh_log("Staged %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));

    return release.TagName;
}

bool Utilities::ApplyPluginUpdates(UserConfig& config, const QString& mergeDirPath)
{
    bool hasUpdates = false;
    for (const auto& plugin : config.GetPlugins())
    {
        const auto tagName = ProcessPluginUpdate(plugin, mergeDirPath);
        if (tagName.isEmpty())
        {
            continue;
        }

        config.SetTag(plugin.PluginId, tagName);
        hasUpdates = true;
    }

    return hasUpdates;
}

bool Utilities::FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath)
{
    if (!config.Save(NICKELUPDATER_CONF))
    {
        nh_log("Failed to save updated tags");
        return false;
    }

    const auto mergedArchivePath = MergedArchivePath();
    if (!CreateArchive(mergeDirPath, mergedArchivePath))
    {
        nh_log("Failed to create merged KoboRoot.tgz");
        return false;
    }

    if (!PublishArchive(mergedArchivePath))
    {
        nh_log("Failed to publish merged KoboRoot.tgz");
        return false;
    }

    if (!RebootDevice())
    {
        nh_log("Failed to reboot after publishing merged KoboRoot.tgz");
        return false;
    }

    nh_log("Published merged KoboRoot.tgz");
    return true;
}

void Utilities::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        nh_log("Failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return; // nothing to do
    }

    if (QFile::copy(tmplFilePath, filePath))
    {
        nh_log("Created config from template: %s", filePath);
    }
    else
    {
        nh_log("Failed to create config from template: %s -> %s", tmplFilePath, filePath);
    }
}

QString Utilities::MergeDirectoryPath()
{
    return QDir(STAGING_DIR).filePath("_merged_root");
}
