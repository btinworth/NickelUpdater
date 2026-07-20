#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

class RuntimeUpdateTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void batchesMultipleUpdatesIntoSingleFinalize();
    void noEffectiveChangeDoesNotFinalize();
    void singlePluginUpdate();
    void partialFailureStillFinalizes();
    void allPluginsFailDoesNotFinalize();
    void configMissingIsCreatedFromTemplate();

private:
    QString CreateArchiveWithFile(const QString& relativePath, const QString& content, const QString& archiveBaseName) const;
    void WriteFakeWgetScript() const;
    void WriteFakeRebootScript() const;
    QString ReadFile(const QString& path) const;
    QStringList TarList(const QString& archivePath) const;
    void WriteConfig(const QString& contents) const;

    QString TempRoot;
    QString BinDir;
    QString DataDir;
    QString OnboardDir;
    QString ConfigPath;
    QString TemplatePath;
    QString StagingDir;

    QByteArray OnboardDirBytes;
    QByteArray ConfigDirBytes;
    QByteArray ConfigPathBytes;
    QByteArray TemplatePathBytes;
    QByteArray StagingDirBytes;
    QByteArray OriginalPath;

    const char* OriginalOnboardDir = nullptr;
    const char* OriginalConfigDir = nullptr;
    const char* OriginalConfigPath = nullptr;
    const char* OriginalTemplatePath = nullptr;
    const char* OriginalStagingDir = nullptr;
};
