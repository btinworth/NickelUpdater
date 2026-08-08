#pragma once

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

    static Result Run(UserConfig& config);

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

    static QString MergeDirectoryPath();
    static bool PrepareMergeDirectory(const QString& mergeDirPath);
    static PluginUpdateResult StagePluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath);
    static QString StageDirectoryForPlugin(const QString& pluginId);
    static bool DownloadFile(const QString& url, const QString& outputPath);
    static bool ExtractArchive(const QString& archivePath, const QString& outputDir);
    static bool PublishMergedUpdate(const UserConfig& config, const QString& mergeDirPath);
    static QString MergedArchivePath();
    static bool CreateArchive(const QString& sourceDir, const QString& archivePath);
    static bool RunProcess(const QString& program, const QStringList& args, QByteArray* output = nullptr);
    static bool PublishArchive(const QString& archivePath);
    static bool RebootDevice();
};
