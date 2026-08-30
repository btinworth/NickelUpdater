#pragma once

#include "HttpClient.h"
#include "UserConfig.h"

class UpdateService
{
public:
    enum class Result
    {
        Failed,
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
    static bool PublishUpdate(const UserConfig& config, const QByteArray& archive);
};
