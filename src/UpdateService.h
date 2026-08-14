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
    };

    static bool PrepareMergeDirectory(const QString& mergeDirPath);
    static PluginUpdateResult StagePluginUpdate(HttpClient& httpClient, const PluginConfigEntry& plugin, const QString& mergeDirPath);
    static bool DownloadFile(HttpClient& httpClient, const QString& url, const QString& outputPath);
    static bool PublishMergedUpdate(const UserConfig& config, const QString& mergeDirPath);
    static bool RunProcess(const QString& program, const QStringList& args, QByteArray* output = nullptr);
};
