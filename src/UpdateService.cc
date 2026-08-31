#include "UpdateService.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "Log.h"
#include "Toast.h"
#include <QCryptographicHash>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <cstring>
#include <unistd.h>

UpdateService::Result UpdateService::Run(UserConfig& config, HttpClient& httpClient)
{
    ResolvePendingUpdate(config);

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

        return PublishUpdate(plugin.PluginId, result.TagName, result.Archive) ? Result::Updated : Result::Failed;
    }

    return hadFailures ? Result::Failed : Result::NoUpdates;
}

void UpdateService::ResolvePendingUpdate(UserConfig& config)
{
    QString pluginId;
    QString tagName;
    if (!ReadPendingUpdate(&pluginId, &tagName))
    {
        return;
    }

    if (QFile::exists(KOBOROOT_PATH))
    {
        // still waiting to be installed
        return;
    }

    config.SetTag(pluginId, tagName);
    if (!config.Save(NICKELUPDATER_CONF))
    {
        Log("Failed to save confirmed update for %s", qPrintable(pluginId));
        return;
    }

    Log("Confirmed %s installed at %s", qPrintable(pluginId), qPrintable(tagName));
    ClearPendingUpdate();
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

    if (!IsValidArchive(release, archive))
    {
        Log("Downloaded KoboRoot.tgz for %s failed validation", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}, {}};
    }

    Log("Downloaded %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
    return {PluginUpdateStatus::Updated, release.TagName, archive};
}

bool UpdateService::IsValidArchive(const PluginRelease& release, const QByteArray& archive)
{
    static const uchar gzipMagic[] = {0x1f, 0x8b};
    if (archive.size() < 2 || memcmp(archive.constData(), gzipMagic, sizeof(gzipMagic)) != 0)
    {
        Log("Archive is not a gzip file");
        return false;
    }

    if (release.Size > 0 && archive.size() != release.Size)
    {
        Log("Archive size %lld does not match expected size %lld", static_cast<long long>(archive.size()), static_cast<long long>(release.Size));
        return false;
    }

    if (!release.Sha256Digest.isEmpty())
    {
        const auto actualDigest = QCryptographicHash::hash(archive, QCryptographicHash::Sha256).toHex();
        if (QString::fromUtf8(actualDigest).compare(release.Sha256Digest, Qt::CaseInsensitive) != 0)
        {
            Log("Archive checksum does not match expected checksum");
            return false;
        }
    }

    return true;
}

bool UpdateService::PublishUpdate(const QString& pluginId, const QString& tagName, const QByteArray& archive)
{
    QSaveFile file(KOBOROOT_PATH);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(archive) != archive.size() || !file.flush())
    {
        Log("Failed to publish KoboRoot.tgz");
        return false;
    }

    // flush to disk before the commit's rename, so a fast reboot can't leave a truncated file
    if (fsync(file.handle()) != 0 || !file.commit())
    {
        Log("Failed to publish KoboRoot.tgz");
        return false;
    }

    // the config tag is only updated once the install is confirmed on a later run, so a bad install can't be recorded as success
    if (!WritePendingUpdate(pluginId, tagName))
    {
        Log("Failed to save pending update state");
        return false;
    }

    // flush the renamed directory entry too, since sync() covers what fsync on the file alone doesn't
    sync();

    // no forced reboot; nickel applies KoboRoot.tgz on the user's next natural reboot
    Log("Published KoboRoot.tgz; will be installed on next reboot");
    ShowToast("NickelUpdater", QString("%1 update ready; will be installed on next reboot").arg(pluginId), 5000);
    return true;
}

bool UpdateService::ReadPendingUpdate(QString* pluginId, QString* tagName)
{
    QFile file(NICKELUPDATER_PENDING);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    const auto lines = QString::fromUtf8(file.readAll()).split('\n');
    if (lines.size() < 2 || lines[0].isEmpty() || lines[1].isEmpty())
    {
        return false;
    }

    *pluginId = lines[0];
    *tagName = lines[1];
    return true;
}

bool UpdateService::WritePendingUpdate(const QString& pluginId, const QString& tagName)
{
    QSaveFile file(NICKELUPDATER_PENDING);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }

    QTextStream out(&file);
    out << pluginId << "\n"
        << tagName << "\n";
    out.flush();

    return file.flush() && fsync(file.handle()) == 0 && file.commit();
}

void UpdateService::ClearPendingUpdate()
{
    QFile::remove(NICKELUPDATER_PENDING);
}
