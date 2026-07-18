#include "NickelUpdater.h"
#include "Constants.h"
#include "PluginRelease.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
{
    CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);
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

    PluginReleaseClient releaseClient;
    for (const auto& plugin : plugins)
    {
        const auto release = releaseClient.GetLatestRelease(plugin.pluginId);
        if (!release.IsValid())
        {
            nh_log("NickelUpdater: failed to load latest release for %s", qPrintable(plugin.pluginId));
            continue;
        }

        nh_log("NickelUpdater: plugin %s installed=%s", qPrintable(plugin.pluginId), qPrintable(plugin.installedVersion));
        nh_log("NickelUpdater: selected release %s for %s", qPrintable(release.tagName), qPrintable(plugin.pluginId));
        nh_log("NickelUpdater: KoboRoot.tgz asset %s", qPrintable(release.koboRootUrl));
        nh_log("NickelUpdater: KoboRoot.tgz checksum %s", qPrintable(release.checksum));
    }

    nh_log("NickelUpdater: update finished");
}

void NickelUpdater::OnNetworkDisconnected()
{
    nh_log("NickelUpdater: WiFi disconnected");
}

void NickelUpdater::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        nh_log("NickelUpdater: failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return; // nothing to do
    }

    if (QFile::copy(tmplFilePath, filePath))
    {
        nh_log("NickelUpdater: created config from template: %s", filePath);
    }
    else
    {
        nh_log("NickelUpdater: failed to create config from template: %s -> %s", tmplFilePath, filePath);
    }
}
