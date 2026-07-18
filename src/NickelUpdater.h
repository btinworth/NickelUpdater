#pragma once

#include <QObject>

extern QObject* (*WirelessManagerInstance)();

class NickelUpdater : public QObject
{
    Q_OBJECT

public:
    NickelUpdater();

public slots:
    void OnNetworkConnected();
    void OnNetworkDisconnected();

private:
    static void CreateConfig(const char* filePath, const char* tmplFilePath);
    static QString ExtractSha256Digest(const QString& digest);
    static bool DownloadFile(const QString& url, const QString& outputPath);
    static bool VerifySha256(const QString& filePath, const QString& expectedHex);
    static QString StageDirectoryForPlugin(const QString& pluginId);
    static QString MergeDirectoryPath();
    static QString MergedArchivePath();
    static bool ExtractArchive(const QString& archivePath, const QString& outputDir);
    static bool CreateArchive(const QString& sourceDir, const QString& archivePath);
    static bool PublishArchive(const QString& archivePath);
    static bool RebootDevice();
};
