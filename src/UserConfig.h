#pragma once

#include <QString>
#include <QVector>

struct PluginConfigEntry
{
    QString pluginId;
    QString installedVersion;
};

class UserConfig
{
public:
    bool Load(const QString& path);
    bool Save(const QString& path) const;

    const QVector<PluginConfigEntry>& GetPlugins() const;

private:
    static QString StripComment(const QString& line);

    QVector<PluginConfigEntry> Plugins;
};
