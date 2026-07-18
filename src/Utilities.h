#pragma once

#include "UserConfig.h"
#include <QString>

class RuntimeUpdateTest;

class Utilities
{
public:
    static QString ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath);

    static bool FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath);
    static bool PrepareMergeDirectory(const QString& mergeDirPath);

    static void CreateConfig(const char* filePath, const char* tmplFilePath);
    static QString MergeDirectoryPath();

private:
    friend class RuntimeUpdateTest;

    static QString ExtractSha256Digest(const QString& digest);
    static bool DownloadFile(const QString& url, const QString& outputPath);
    static bool VerifySha256(const QString& filePath, const QString& expectedHex);
    static QString StageDirectoryForPlugin(const QString& pluginId);
    static QString MergedArchivePath();
    static bool ExtractArchive(const QString& archivePath, const QString& outputDir);
    static bool CreateArchive(const QString& sourceDir, const QString& archivePath);
    static bool PublishArchive(const QString& archivePath);
    static bool RebootDevice();
    static bool EnsureMergeDirectoryReady(const QString& mergeDirPath);
};
