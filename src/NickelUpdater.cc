#include "NickelUpdater.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "Utilities.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QFileInfo>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
{
    Utilities::CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);
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

    const auto mergeDirPath = Utilities::MergeDirectoryPath();
    if (!Utilities::PrepareMergeDirectory(mergeDirPath))
    {
        return;
    }

    bool hasUpdates = false;
    for (const auto& plugin : plugins)
    {
        const auto tagName = Utilities::ProcessPluginUpdate(plugin, mergeDirPath);
        if (tagName.isEmpty())
        {
            continue;
        }

        if (!config.SetTag(plugin.PluginId, tagName))
        {
            nh_log("NickelUpdater: failed to update tag for %s", qPrintable(plugin.PluginId));
            return;
        }

        hasUpdates = true;
    }

    if (!hasUpdates)
    {
        nh_log("NickelUpdater: no updates to apply");
        nh_log("NickelUpdater: update finished");
        return;
    }

    if (!Utilities::FinalizeAndApplyUpdates(config, mergeDirPath))
    {
        return;
    }

    nh_log("NickelUpdater: update finished");
}

void NickelUpdater::OnNetworkDisconnected()
{
    nh_log("NickelUpdater: WiFi disconnected");
}
