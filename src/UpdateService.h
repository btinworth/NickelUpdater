#pragma once

#include "HttpClient.h"
#include "PluginRelease.h"
#include "UserConfig.h"

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
    static bool PublishUpdate(const UserConfig& config, const QByteArray& archive);
};
