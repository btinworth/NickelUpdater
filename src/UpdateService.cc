#include "UpdateService.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "Log.h"
#include <QFile>
#include <QProcess>
#include <QSaveFile>

UpdateService::Result UpdateService::Run(UserConfig& config, HttpClient& httpClient)
{
    // defer if KoboRoot.tgz already exists
    if (QFile::exists(KOBOROOT_PATH))
    {
        Log("KoboRoot.tgz is already pending install; deferring update");
        return Result::Deferred;
    }

    bool hadFailures = false;
    // plugins are checked in the order they appear in the config, and only the first one with an update is applied
    for (const auto& plugin : config.GetPlugins())
    {
        const auto result = DownloadPluginUpdate(httpClient, plugin);
        if (result.Status == PluginUpdateStatus::Failed)
        {
            hadFailures = true;
            continue;
        }

        if (result.Status == PluginUpdateStatus::Unchanged)
        {
            continue;
        }

        // publishing reboots the device, so never do it from a half finished download
        if (httpClient.IsSessionCanceled())
        {
            Log("Session canceled; not publishing update for %s", qPrintable(plugin.PluginId));
            return Result::Failed;
        }

        config.SetTag(plugin.PluginId, result.TagName);
        return PublishUpdate(config, result.Archive) ? Result::Updated : Result::Failed;
    }

    return hadFailures ? Result::Failed : Result::NoUpdates;
}

UpdateService::PluginUpdateResult UpdateService::DownloadPluginUpdate(HttpClient& httpClient, const PluginConfigEntry& plugin)
{
    const auto release = GitHubInterface::GetLatestRelease(httpClient, plugin.PluginId);
    if (!release.IsValid())
    {
        Log("Failed to load latest release for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}, {}};
    }

    if (!plugin.TagName.isEmpty() && plugin.TagName == release.TagName)
    {
        Log("Plugin %s already at %s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));
        return {PluginUpdateStatus::Unchanged, {}, {}};
    }

    QByteArray archive;
    if (!httpClient.Get(release.KoboRootUrl, &archive, "*/*"))
    {
        Log("Failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}, {}};
    }

    Log("Downloaded %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
    return {PluginUpdateStatus::Updated, release.TagName, archive};
}

bool UpdateService::PublishUpdate(const UserConfig& config, const QByteArray& archive)
{
    QSaveFile file(KOBOROOT_PATH);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(archive) != archive.size() || !file.commit())
    {
        Log("Failed to publish KoboRoot.tgz");
        return false;
    }

    if (!config.Save(NICKELUPDATER_CONF))
    {
        Log("Failed to save updated tags");
        return false;
    }

    if (!QProcess::startDetached("reboot"))
    {
        Log("Failed to reboot after publishing KoboRoot.tgz");
        return false;
    }

    Log("Published KoboRoot.tgz");
    return true;
}
