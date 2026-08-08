#include "NickelUpdater.h"
#include "Constants.h"
#include "UpdateService.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QDir>
#include <QFile>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
    : IsUpdating(false)
{
    CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);
}

void NickelUpdater::OnNetworkConnected()
{
    if (IsUpdating)
    {
        nh_log("Update already in progress; skipping new network-connected trigger");
        return;
    }

    IsUpdating = true;
    Client.BeginSession();

    nh_log("Starting update");

    auto result = UpdateService::Result::Failed;
    UserConfig config;
    if (!config.Load(NICKELUPDATER_CONF))
    {
        nh_log("Failed to open config: %s", NICKELUPDATER_CONF);
    }
    else
    {
        nh_log("Config loaded from %s (%lld plugin(s))", NICKELUPDATER_CONF, static_cast<long long>(config.GetPlugins().size()));
        result = UpdateService::Run(config, Client);
    }

    switch (result)
    {
    case UpdateService::Result::Failed:
        break;
    case UpdateService::Result::NoUpdates:
        nh_log("No updates to apply");
        break;
    case UpdateService::Result::Updated:
        nh_log("Update finished");
        break;
    default:
        break;
    }

    IsUpdating = false;
}

void NickelUpdater::OnNetworkDisconnected()
{
    Client.CancelSession();

    if (IsUpdating)
    {
        nh_log("Network disconnected; canceling active update");
    }
}

void NickelUpdater::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        nh_log("Failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return;
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
