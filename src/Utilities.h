#pragma once

#include "UserConfig.h"
#include <QByteArray>
#include <QString>
#include <QStringList>

class Utilities
{
public:
    static QString ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath);

    static bool FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath);
    static bool PrepareMergeDirectory(const QString& mergeDirPath);

    static void CreateConfig(const char* filePath, const char* tmplFilePath);
    static QString MergeDirectoryPath();

    // Runs `program` with `args` to completion; on success, captures stdout in `output` if given.
    static bool RunProcess(const QString& program, const QStringList& args, QByteArray* output = nullptr);

private:
    static bool DownloadFile(const QString& url, const QString& outputPath);
    static QString StageDirectoryForPlugin(const QString& pluginId);
    static QString MergedArchivePath();
    static bool ExtractArchive(const QString& archivePath, const QString& outputDir);
    static bool CreateArchive(const QString& sourceDir, const QString& archivePath);
    static bool PublishArchive(const QString& archivePath);
    static bool RebootDevice();
};
