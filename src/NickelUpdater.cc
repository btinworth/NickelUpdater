#include "NickelUpdater.h"
#include "Constants.h"
#include "Log.h"
#include "UpdateService.h"
#include "UserConfig.h"
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
        Log("Update already in progress; skipping new network-connected trigger");
        return;
    }

    IsUpdating = true;
    Client.BeginSession();

    Log("Starting update");

    auto result = UpdateService::Result::Failed;
    UserConfig config;
    if (!config.Load(NICKELUPDATER_CONF))
    {
        Log("Failed to open config: %s", NICKELUPDATER_CONF);
    }
    else
    {
        Log("Config loaded from %s (%lld plugin(s))", NICKELUPDATER_CONF, static_cast<long long>(config.GetPlugins().size()));
        result = UpdateService::Run(config, Client);
    }

    switch (result)
    {
    case UpdateService::Result::Failed:
        break;
    case UpdateService::Result::NoUpdates:
        Log("No updates to apply");
        break;
    case UpdateService::Result::Updated:
        Log("Update finished");
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
        Log("Network disconnected; canceling active update");
    }
}

void NickelUpdater::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        Log("Failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return;
    }

    if (QFile::copy(tmplFilePath, filePath))
    {
        Log("Created config from template: %s", filePath);
    }
    else
    {
        Log("Failed to create config from template: %s -> %s", tmplFilePath, filePath);
    }
}
