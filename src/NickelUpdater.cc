#include "NickelUpdater.h"
#include "Constants.h"
#include "UpdateService.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QDir>
#include <QFile>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
{
    CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);
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

    switch (UpdateService::Run(config))
    {
    case UpdateService::Result::Failed:
        return;
    case UpdateService::Result::NoUpdates:
        nh_log("No updates to apply");
        return;
    case UpdateService::Result::Updated:
        break;
    default:
        return;
    }

    nh_log("Update finished");
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
