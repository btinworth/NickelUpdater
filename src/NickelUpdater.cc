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

    nh_log("Config loaded from %s (%lld plugin(s))", NICKELUPDATER_CONF, static_cast<long long>(config.GetPlugins().size()));

    const auto mergeDirPath = Utilities::MergeDirectoryPath();
    if (!Utilities::PrepareMergeDirectory(mergeDirPath))
    {
        return;
    }

    if (!Utilities::ApplyPluginUpdates(config, mergeDirPath))
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
