#pragma once

#include <QString>
#include <QVector>

struct PluginConfigEntry
{
    QString PluginId;
    QString TagName;
};

class UserConfig
{
public:
    bool Load(const QString& path);
    bool Save(const QString& path) const;
    bool SetTag(const QString& pluginId, const QString& tagName);

    const QVector<PluginConfigEntry>& GetPlugins() const;

private:
    static QString StripComment(const QString& line);

    QVector<PluginConfigEntry> Plugins;
};
