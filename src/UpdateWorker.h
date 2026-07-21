#pragma once

#include "UserConfig.h"
#include <QObject>

class UpdateWorker : public QObject
{
    Q_OBJECT

public slots:
    void Run();

signals:
    void Finished(bool hasUpdates);

private:
    bool PrepareMergeDirectory(const QString& mergeDirPath);

    bool ApplyPluginUpdates(UserConfig& config, const QString& mergeDirPath);
    QString ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath);

    bool FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath);
};
