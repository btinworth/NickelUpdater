#pragma once

#include "UserConfig.h"
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>

class Utilities
{
public:
    static bool ApplyPluginUpdates(UserConfig& config, const QString& mergeDirPath);

    static bool FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath);
    static bool PrepareMergeDirectory(const QString& mergeDirPath);

    static void CreateConfig(const char* filePath, const char* tmplFilePath);
    static QString MergeDirectoryPath();

    static bool RunProcess(const QString& program, const QStringList& args, QByteArray* output = nullptr);

    static bool HttpGet(const QString& url, QByteArray* output);

private:
    static QString ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath);
    static bool DownloadFile(const QString& url, const QString& outputPath);
    static QString StageDirectoryForPlugin(const QString& pluginId);
    static QString MergedArchivePath();
    static bool ExtractArchive(const QString& archivePath, const QString& outputDir);
    static bool CreateArchive(const QString& sourceDir, const QString& archivePath);
    static bool PublishArchive(const QString& archivePath);
    static bool RebootDevice();
};
