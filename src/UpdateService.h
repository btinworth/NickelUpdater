#pragma once

#include "HttpClient.h"
#include "PluginRelease.h"
#include "UserConfig.h"
#include <QMetaType>
#include <functional>

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

    // primary, secondary, milliseconds - called from the worker thread, must only emit a queued signal back to the GUI thread
    using ToastCallback = std::function<void(const QString&, const QString&, int)>;

    static Result Run(UserConfig& config, HttpClient& httpClient, const ToastCallback& requestToast);

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
    static bool PublishUpdate(const QString& pluginId, const QString& tagName, const QByteArray& archive, const ToastCallback& requestToast);

    // an update is only recorded as installed once KoboRoot.tgz is confirmed gone on a later run
    static void ResolvePendingUpdate(UserConfig& config);
    static bool ReadPendingUpdate(QString* pluginId, QString* tagName);
    static bool WritePendingUpdate(const QString& pluginId, const QString& tagName);
    static void ClearPendingUpdate();
};

Q_DECLARE_METATYPE(UpdateService::Result)
