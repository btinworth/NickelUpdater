#include "NickelUpdater.h"
#include "Constants.h"
#include "PluginRelease.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QProcess>

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
        const auto release = releaseClient.GetLatestRelease(plugin.PluginId);
        if (!release.IsValid())
        {
            nh_log("NickelUpdater: failed to load latest release for %s", qPrintable(plugin.PluginId));
            continue;
        }

        nh_log("NickelUpdater: plugin %s installed=%s", qPrintable(plugin.PluginId), qPrintable(plugin.InstalledVersion));

        const auto stageDirPath = StageDirectoryForPlugin(plugin.PluginId);
        if (!QDir().mkpath(stageDirPath))
        {
            nh_log("NickelUpdater: failed to create stage dir for %s", qPrintable(plugin.PluginId));
            continue;
        }

        const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
        if (!DownloadFile(release.KoboRootUrl, stageFilePath))
        {
            nh_log("NickelUpdater: failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
            continue;
        }

        const auto expectedDigest = ExtractSha256Digest(release.Checksum);
        if (expectedDigest.isEmpty() || !VerifySha256(stageFilePath, expectedDigest))
        {
            nh_log("NickelUpdater: checksum mismatch for %s", qPrintable(plugin.PluginId));
            QFile::remove(stageFilePath);
            continue;
        }

        const auto onboardFilePath = QDir(ONBOARD_DIR).filePath("KoboRoot.tgz");
        QFile::remove(onboardFilePath);
        if (!QFile::copy(stageFilePath, onboardFilePath))
        {
            nh_log("NickelUpdater: failed to publish KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
            continue;
        }

        nh_log("NickelUpdater: selected release %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
        nh_log("NickelUpdater: staged KoboRoot.tgz at %s", qPrintable(stageFilePath));
        nh_log("NickelUpdater: published KoboRoot.tgz to %s", qPrintable(onboardFilePath));
        nh_log("NickelUpdater: KoboRoot.tgz checksum %s", qPrintable(release.Checksum));
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

QString NickelUpdater::ExtractSha256Digest(const QString& digest)
{
    const int colon = digest.indexOf(':');
    if (colon < 0)
    {
        return {};
    }

    return digest.mid(colon + 1).trimmed();
}

bool NickelUpdater::DownloadFile(const QString& url, const QString& outputPath)
{
    QProcess curl;
    curl.start("curl", QStringList{
        "-fsSL",
        "-L",
        "-H", "User-Agent: NickelUpdater",
        "-H", "Accept: application/vnd.github+json",
        "-o", outputPath,
        url,
    });
    return curl.waitForFinished() && curl.exitStatus() == QProcess::NormalExit && curl.exitCode() == 0;
}

bool NickelUpdater::VerifySha256(const QString& filePath, const QString& expectedHex)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
    {
        hash.addData(file.read(64 * 1024));
    }

    return QString::fromLatin1(hash.result().toHex()) == expectedHex.toLower();
}

QString NickelUpdater::StageDirectoryForPlugin(const QString& pluginId)
{
    return QDir(CONFIG_DIR).filePath(QString("staging/%1").arg(pluginId));
}
