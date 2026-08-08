#include "UpdateService.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "PluginRelease.h"
#include "Utilities.h"
#include <NickelHook.h>
#include <QDir>
#include <QFile>
#include <QProcess>

UpdateService::Result UpdateService::Run(UserConfig& config)
{
    const auto mergeDirPath = MergeDirectoryPath();
    if (!PrepareMergeDirectory(mergeDirPath))
    {
        return Result::Failed;
    }

    bool hasUpdates = false;
    for (const auto& plugin : config.GetPlugins())
    {
        const auto result = StagePluginUpdate(plugin, mergeDirPath);
        if (result.Status == PluginUpdateStatus::Failed)
        {
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
        return Result::NoUpdates;
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

UpdateService::PluginUpdateResult UpdateService::StagePluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath)
{
    const auto release = GitHubInterface::GetLatestRelease(plugin.PluginId);
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
    if (!DownloadFile(release.KoboRootUrl, stageFilePath))
    {
        nh_log("Failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}};
    }

    if (!ExtractArchive(stageFilePath, mergeDirPath))
    {
        nh_log("Failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}};
    }

    nh_log("Staged %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
    return {PluginUpdateStatus::Updated, release.TagName};
}

QString UpdateService::StageDirectoryForPlugin(const QString& pluginId)
{
    return QDir(STAGING_DIR).filePath(pluginId);
}

bool UpdateService::DownloadFile(const QString& url, const QString& outputPath)
{
    QByteArray output;
    if (!Utilities::HttpGet(url, &output, "*/*"))
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

bool UpdateService::ExtractArchive(const QString& archivePath, const QString& outputDir)
{
    return Utilities::RunProcess("tar", {"-xzf", archivePath, "-C", outputDir});
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
    return Utilities::RunProcess("tar", {"-czf", archivePath, "-C", sourceDir, "."});
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
