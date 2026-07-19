#include "NickelUpdater.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "UserConfig.h"
#include "Utilities.h"
#include <NickelHook.h>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
{
    Utilities::CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);
}

void NickelUpdater::OnNetworkConnected()
{
    nh_log("Starting update");

    UserConfig config;
    if (!config.Load(NICKELUPDATER_CONF))
    {
        nh_log("Failed to open config: %s", NICKELUPDATER_CONF);
        return;
    }

    const auto& plugins = config.GetPlugins();
    nh_log("Config loaded from %s (%lld plugin(s))", NICKELUPDATER_CONF, static_cast<long long>(plugins.size()));

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
