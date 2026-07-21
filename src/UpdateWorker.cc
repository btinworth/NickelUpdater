#include "UpdateWorker.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "UserConfig.h"
#include "Utilities.h"
#include <NickelHook.h>
#include <QDir>

bool UpdateWorker::PrepareMergeDirectory(const QString& mergeDirPath)
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

QString UpdateWorker::ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath)
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

    const auto stageDirPath = Utilities::StageDirectoryForPlugin(plugin.PluginId);
    if (!QDir().mkpath(stageDirPath))
    {
        nh_log("Failed to create stage dir for %s", qPrintable(plugin.PluginId));
        return {};
    }

    const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
    if (!Utilities::DownloadFile(release.KoboRootUrl, stageFilePath))
    {
        nh_log("Failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    if (!Utilities::ExtractArchive(stageFilePath, mergeDirPath))
    {
        nh_log("Failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    nh_log("Staged %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));

    return release.TagName;
}

bool UpdateWorker::ApplyPluginUpdates(UserConfig& config, const QString& mergeDirPath)
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

bool UpdateWorker::FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath)
{
    if (!config.Save(NICKELUPDATER_CONF))
    {
        nh_log("Failed to save updated tags");
        return false;
    }

    const auto mergedArchivePath = Utilities::MergedArchivePath();
    if (!Utilities::CreateArchive(mergeDirPath, mergedArchivePath))
    {
        nh_log("Failed to create merged KoboRoot.tgz");
        return false;
    }

    if (!Utilities::PublishArchive(mergedArchivePath))
    {
        nh_log("Failed to publish merged KoboRoot.tgz");
        return false;
    }

    if (!Utilities::RebootDevice())
    {
        nh_log("Failed to reboot after publishing merged KoboRoot.tgz");
        return false;
    }

    nh_log("Published merged KoboRoot.tgz");
    return true;
}

void UpdateWorker::Run()
{
    nh_log("Starting update");

    UserConfig config;
    if (!config.Load(NICKELUPDATER_CONF))
    {
        nh_log("Failed to open config: %s", NICKELUPDATER_CONF);
        emit Finished(false);
        return;
    }

    nh_log("Config loaded from %s (%lld plugin(s))", NICKELUPDATER_CONF, static_cast<long long>(config.GetPlugins().size()));

    const auto mergeDirPath = Utilities::MergeDirectoryPath();
    if (!PrepareMergeDirectory(mergeDirPath))
    {
        emit Finished(false);
        return;
    }

    if (!ApplyPluginUpdates(config, mergeDirPath))
    {
        nh_log("No updates to apply");
        emit Finished(false);
        return;
    }

    if (!FinalizeAndApplyUpdates(config, mergeDirPath))
    {
        emit Finished(false);
        return;
    }

    nh_log("Update finished");
    emit Finished(true);
}

