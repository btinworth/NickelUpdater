#include "UpdateService.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include <NickelHook.h>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QProcess>

UpdateService::Result UpdateService::Run(UserConfig& config, HttpClient& httpClient)
{
    const auto mergeDirPath = MergeDirectoryPath();
    if (!PrepareMergeDirectory(mergeDirPath))
    {
        return Result::Failed;
    }

    bool hasUpdates = false;
    bool hadFailures = false;
    for (const auto& plugin : config.GetPlugins())
    {
        const auto result = StagePluginUpdate(httpClient, plugin, mergeDirPath);
        if (result.Status == PluginUpdateStatus::Failed)
        {
            hadFailures = true;
            continue;
        }

        if (result.Status == PluginUpdateStatus::Unchanged)
        {
            continue;
        }

        config.SetTag(plugin.PluginId, result.TagName);
        hasUpdates = true;
    }

    if (!hasUpdates)
    {
        return hadFailures ? Result::Failed : Result::NoUpdates;
    }

    return PublishMergedUpdate(config, mergeDirPath) ? Result::Updated : Result::Failed;
}

QString UpdateService::MergeDirectoryPath()
{
    return QDir(STAGING_DIR).filePath("_merged_root");
}

bool UpdateService::PrepareMergeDirectory(const QString& mergeDirPath)
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

UpdateService::PluginUpdateResult UpdateService::StagePluginUpdate(HttpClient& httpClient, const PluginConfigEntry& plugin, const QString& mergeDirPath)
{
    const auto release = GitHubInterface::GetLatestRelease(httpClient, plugin.PluginId);
    if (!release.IsValid())
    {
        nh_log("Failed to load latest release for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}};
    }

    if (!plugin.TagName.isEmpty() && plugin.TagName == release.TagName)
    {
        nh_log("Plugin %s already at %s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));
        return {PluginUpdateStatus::Unchanged, {}};
    }

    const auto stageDirPath = StageDirectoryForPlugin(plugin.PluginId);
    if (!QDir().mkpath(stageDirPath))
    {
        nh_log("Failed to create stage dir for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}};
    }

    const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
    if (!DownloadFile(httpClient, release.KoboRootUrl, stageFilePath, release.AssetDigest))
    {
        nh_log("Failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        QDir(stageDirPath).removeRecursively();
        return {PluginUpdateStatus::Failed, {}};
    }

    if (!ExtractArchive(stageFilePath, mergeDirPath))
    {
        nh_log("Failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        QDir(stageDirPath).removeRecursively();
        return {PluginUpdateStatus::Failed, {}};
    }

    nh_log("Staged %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
    return {PluginUpdateStatus::Updated, release.TagName};
}

QString UpdateService::StageDirectoryForPlugin(const QString& pluginId)
{
    return QDir(STAGING_DIR).filePath(pluginId);
}

bool UpdateService::DownloadFile(HttpClient& httpClient, const QString& url, const QString& outputPath, const QString& expectedDigest)
{
    QByteArray output;
    if (!httpClient.Get(url, &output, "*/*"))
    {
        return false;
    }

    if (!VerifyDigest(output, expectedDigest))
    {
        nh_log("Digest mismatch for %s", qPrintable(url));
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

bool UpdateService::VerifyDigest(const QByteArray& data, const QString& expectedDigest)
{
    if (expectedDigest.isEmpty())
    {
        return true;
    }

    const auto separator = expectedDigest.indexOf(':');
    if (separator < 0)
    {
        return false;
    }

    const auto algorithm = expectedDigest.left(separator);
    const auto expectedHex = expectedDigest.mid(separator + 1);
    if (algorithm != "sha256")
    {
        nh_log("Unsupported digest algorithm: %s", qPrintable(algorithm));
        return false;
    }

    const auto actualHex = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(actualHex).compare(expectedHex, Qt::CaseInsensitive) == 0;
}

bool UpdateService::ExtractArchive(const QString& archivePath, const QString& outputDir)
{
    return RunProcess("tar", {"-xzf", archivePath, "-C", outputDir});
}

bool UpdateService::PublishMergedUpdate(const UserConfig& config, const QString& mergeDirPath)
{
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

    if (!config.Save(NICKELUPDATER_CONF))
    {
        nh_log("Failed to save updated tags");
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

QString UpdateService::MergedArchivePath()
{
    return QDir(STAGING_DIR).filePath("KoboRoot.merged.tgz");
}

bool UpdateService::CreateArchive(const QString& sourceDir, const QString& archivePath)
{
    QFile::remove(archivePath);
    return RunProcess("tar", {"-czf", archivePath, "-C", sourceDir, "."});
}

bool UpdateService::RunProcess(const QString& program, const QStringList& args, QByteArray* output)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        nh_log("%s %s failed (exit code %d): %s", qPrintable(program), qPrintable(args.join(' ')), process.exitCode(), process.readAllStandardError().constData());
        return false;
    }

    if (output != nullptr)
    {
        *output = process.readAllStandardOutput();
    }

    return true;
}

bool UpdateService::PublishArchive(const QString& archivePath)
{
    QFile::remove(KOBOROOT_PATH);
    return QFile::copy(archivePath, KOBOROOT_PATH);
}

bool UpdateService::RebootDevice()
{
    return QProcess::startDetached("reboot");
}
