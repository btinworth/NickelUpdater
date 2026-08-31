#pragma once

#include "HttpClient.h"
#include "PluginRelease.h"
#include "UserConfig.h"
#include <QMetaType>

class UpdateService
{
public:
    enum class Result
    {
        Failed,
        Deferred,
        NoUpdates,
        Updated,
    };

    static Result Run(UserConfig& config, HttpClient& httpClient);

private:
    enum class PluginUpdateStatus
    {
        Failed,
        Unchanged,
        Updated,
    };

    struct PluginUpdateResult
    {
        PluginUpdateStatus Status;
        QString TagName;
        QByteArray Archive;
    };

    static PluginUpdateResult DownloadPluginUpdate(HttpClient& httpClient, const PluginConfigEntry& plugin);
    static bool IsValidArchive(const PluginRelease& release, const QByteArray& archive);
    static bool PublishUpdate(const QString& pluginId, const QString& tagName, const QByteArray& archive);

    // an update is only recorded as installed once KoboRoot.tgz is confirmed gone on a later run
    static void ResolvePendingUpdate(UserConfig& config);
    static bool ReadPendingUpdate(QString* pluginId, QString* tagName);
    static bool WritePendingUpdate(const QString& pluginId, const QString& tagName);
    static void ClearPendingUpdate();
};

Q_DECLARE_METATYPE(UpdateService::Result)
