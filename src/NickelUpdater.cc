#include "NickelUpdater.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "UserConfig.h"
#include "Utilities.h"
#include <NickelHook.h>
#include <QFileInfo>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
{
    Utilities::CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);
}

void NickelUpdater::OnNetworkConnected()
{
    nh_log("WiFi connected");

    QFileInfo configFile(NICKELUPDATER_CONF);
    if (!configFile.exists())
    {
        nh_log("Config does not exist: %s", NICKELUPDATER_CONF);
        return;
    }

    nh_log("Starting update");

    UserConfig config;
    if (!config.Load(NICKELUPDATER_CONF))
    {
        nh_log("Failed to parse config");
        return;
    }

    nh_log("Config loaded from %s", NICKELUPDATER_CONF);

    const auto& plugins = config.GetPlugins();
    nh_log("Found %lld plugin(s) in config", static_cast<long long>(plugins.size()));

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

        config.SetTag(plugin.PluginId, tagName);
        hasUpdates = true;
    }

    if (!hasUpdates)
    {
        nh_log("No updates to apply");
        return;
    }

    if (!Utilities::FinalizeAndApplyUpdates(config, mergeDirPath))
    {
        return;
    }

    nh_log("Update finished");
}

void NickelUpdater::OnNetworkDisconnected()
{
    nh_log("WiFi disconnected");
}
