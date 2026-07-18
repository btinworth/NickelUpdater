#include "NickelUpdater.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QProcess>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
{
    CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);
}

void NickelUpdater::OnNetworkConnected()
{
    nh_log("NickelUpdater: WiFi connected");

    QFileInfo configFile(NICKELUPDATER_CONF);
    if (!configFile.exists())
    {
        nh_log("NickelUpdater: config does not exist: %s", NICKELUPDATER_CONF);
        return;
    }

    nh_log("NickelUpdater: starting update");

    nh_log("NickelUpdater: config loaded from %s", NICKELUPDATER_CONF);

    UserConfig config;
    if (!config.Load(NICKELUPDATER_CONF))
    {
        nh_log("NickelUpdater: failed to parse config");
        return;
    }

    const auto& plugins = config.GetPlugins();
    nh_log("NickelUpdater: found %lld plugin(s) in config", static_cast<long long>(plugins.size()));

    bool hasUpdates = false;
    const auto mergeDirPath = MergeDirectoryPath();

    GitHubInterface releaseClient;
    for (const auto& plugin : plugins)
    {
        if (!ProcessPluginUpdate(config, plugin, releaseClient, mergeDirPath, hasUpdates))
        {
            return;
        }
    }

    if (!hasUpdates)
    {
        nh_log("NickelUpdater: no updates to apply");
        nh_log("NickelUpdater: update finished");
        return;
    }

    if (!FinalizeAndApplyUpdates(config, mergeDirPath))
    {
        return;
    }

    nh_log("NickelUpdater: update finished");
}

void NickelUpdater::OnNetworkDisconnected()
{
    nh_log("NickelUpdater: WiFi disconnected");
}

void NickelUpdater::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        nh_log("NickelUpdater: failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return; // nothing to do
    }

    if (QFile::copy(tmplFilePath, filePath))
    {
        nh_log("NickelUpdater: created config from template: %s", filePath);
    }
    else
    {
        nh_log("NickelUpdater: failed to create config from template: %s -> %s", tmplFilePath, filePath);
    }
}

bool NickelUpdater::ProcessPluginUpdate(
    UserConfig& config,
    const PluginConfigEntry& plugin,
    GitHubInterface& releaseClient,
    const QString& mergeDirPath,
    bool& hasUpdates)
{
    const auto release = releaseClient.GetLatestRelease(plugin.PluginId);
    if (!release.IsValid())
    {
        nh_log("NickelUpdater: failed to load latest release for %s", qPrintable(plugin.PluginId));
        return true;
    }

    if (!plugin.TagName.isEmpty() && plugin.TagName == release.TagName)
    {
        nh_log("NickelUpdater: plugin %s already at %s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));
        return true;
    }

    nh_log("NickelUpdater: plugin %s installed=%s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));

    const auto stageDirPath = StageDirectoryForPlugin(plugin.PluginId);
    if (!QDir().mkpath(stageDirPath))
    {
        nh_log("NickelUpdater: failed to create stage dir for %s", qPrintable(plugin.PluginId));
        return true;
    }

    const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
    if (!DownloadFile(release.KoboRootUrl, stageFilePath))
    {
        nh_log("NickelUpdater: failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return true;
    }

    const auto expectedDigest = ExtractSha256Digest(release.Checksum);
    if (expectedDigest.isEmpty() || !VerifySha256(stageFilePath, expectedDigest))
    {
        nh_log("NickelUpdater: checksum mismatch for %s", qPrintable(plugin.PluginId));
        QFile::remove(stageFilePath);
        return true;
    }

    if (!hasUpdates && !EnsureMergeDirectoryReady(mergeDirPath))
    {
        return false;
    }

    if (!ExtractArchive(stageFilePath, mergeDirPath))
    {
        nh_log("NickelUpdater: failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return true;
    }

    if (!config.SetTag(plugin.PluginId, release.TagName))
    {
        nh_log("NickelUpdater: failed to update tag for %s", qPrintable(plugin.PluginId));
        return true;
    }

    hasUpdates = true;

    nh_log("NickelUpdater: selected release %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
    nh_log("NickelUpdater: staged KoboRoot.tgz at %s", qPrintable(stageFilePath));
    nh_log("NickelUpdater: KoboRoot.tgz checksum %s", qPrintable(release.Checksum));
    return true;
}

bool NickelUpdater::EnsureMergeDirectoryReady(const QString& mergeDirPath)
{
    QDir mergeDir(mergeDirPath);
    if (mergeDir.exists() && !mergeDir.removeRecursively())
    {
        nh_log("NickelUpdater: failed to clear merge directory: %s", qPrintable(mergeDirPath));
        return false;
    }

    if (!QDir().mkpath(mergeDirPath))
    {
        nh_log("NickelUpdater: failed to create merge directory: %s", qPrintable(mergeDirPath));
        return false;
    }

    return true;
}

bool NickelUpdater::FinalizeAndApplyUpdates(UserConfig& config, const QString& mergeDirPath)
{
    if (!config.Save(NICKELUPDATER_CONF))
    {
        nh_log("NickelUpdater: failed to save updated tags");
        return false;
    }

    const auto mergedArchivePath = MergedArchivePath();
    if (!CreateArchive(mergeDirPath, mergedArchivePath))
    {
        nh_log("NickelUpdater: failed to create merged KoboRoot.tgz");
        return false;
    }

    if (!PublishArchive(mergedArchivePath))
    {
        nh_log("NickelUpdater: failed to publish merged KoboRoot.tgz");
        return false;
    }

    if (!RebootDevice())
    {
        nh_log("NickelUpdater: failed to reboot after publishing merged KoboRoot.tgz");
        return false;
    }

    nh_log("NickelUpdater: published merged KoboRoot.tgz");
    return true;
}

QString NickelUpdater::ExtractSha256Digest(const QString& digest)
{
    const int colon = digest.indexOf(':');
    if (colon < 0)
    {
        return {};
    }

    return digest.mid(colon + 1).trimmed();
}

bool NickelUpdater::DownloadFile(const QString& url, const QString& outputPath)
{
    QProcess curl;
    curl.start("curl", QStringList{
        "-fsSL",
        "-L",
        "-H", "User-Agent: NickelUpdater",
        "-H", "Accept: application/vnd.github+json",
        "-o", outputPath,
        url,
    });
    return curl.waitForFinished() && curl.exitStatus() == QProcess::NormalExit && curl.exitCode() == 0;
}

bool NickelUpdater::VerifySha256(const QString& filePath, const QString& expectedHex)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
    {
        hash.addData(file.read(64 * 1024));
    }

    return QString::fromLatin1(hash.result().toHex()) == expectedHex.toLower();
}

QString NickelUpdater::StageDirectoryForPlugin(const QString& pluginId)
{
    return QDir(CONFIG_DIR).filePath(QString("staging/%1").arg(pluginId));
}

QString NickelUpdater::MergeDirectoryPath()
{
    return QDir(CONFIG_DIR).filePath("staging/_merged_root");
}

QString NickelUpdater::MergedArchivePath()
{
    return QDir(CONFIG_DIR).filePath("staging/KoboRoot.merged.tgz");
}

bool NickelUpdater::ExtractArchive(const QString& archivePath, const QString& outputDir)
{
    QProcess tar;
    tar.start("tar", QStringList{
        "-xzf",
        archivePath,
        "-C",
        outputDir,
    });
    return tar.waitForFinished() && tar.exitStatus() == QProcess::NormalExit && tar.exitCode() == 0;
}

bool NickelUpdater::CreateArchive(const QString& sourceDir, const QString& archivePath)
{
    QFile::remove(archivePath);

    QProcess tar;
    tar.start("tar", QStringList{
        "-czf",
        archivePath,
        "-C",
        sourceDir,
        ".",
    });
    return tar.waitForFinished() && tar.exitStatus() == QProcess::NormalExit && tar.exitCode() == 0;
}

bool NickelUpdater::PublishArchive(const QString& archivePath)
{
    const auto onboardFilePath = QDir(ONBOARD_DIR).filePath("KoboRoot.tgz");
    QFile::remove(onboardFilePath);
    return QFile::copy(archivePath, onboardFilePath);
}

bool NickelUpdater::RebootDevice()
{
    return QProcess::startDetached("reboot");
}
