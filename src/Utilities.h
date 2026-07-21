#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>

class Utilities
{
public:
    static void CreateConfig(const char* filePath, const char* tmplFilePath);
    static QString MergeDirectoryPath();
    static QString StageDirectoryForPlugin(const QString& pluginId);
    static QString MergedArchivePath();

    static bool RunProcess(const QString& program, const QStringList& args, QByteArray* output = nullptr);

    static bool HttpGet(const QString& url, QByteArray* output);

    static bool DownloadFile(const QString& url, const QString& outputPath);
    static bool ExtractArchive(const QString& archivePath, const QString& outputDir);
    static bool CreateArchive(const QString& sourceDir, const QString& archivePath);
    static bool PublishArchive(const QString& archivePath);
    static bool RebootDevice();
};

