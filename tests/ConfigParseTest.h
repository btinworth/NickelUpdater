#pragma once

#include "UserConfig.h"
#include <QObject>

class ConfigParseTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidPluginRows();
    void acceptsArbitraryPluginIds();
    void saveWritesDeterministicFormat();
    void updatesInstalledVersion();

private:
    static UserConfig LoadConfig(const QString& contents);
};
