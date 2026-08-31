#pragma once

#include <QMetaType>
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
    void SetTag(const QString& pluginId, const QString& tagName);

    const QVector<PluginConfigEntry>& GetPlugins() const;

private:
    static QString StripComment(const QString& line);

    QVector<PluginConfigEntry> Plugins;
};

Q_DECLARE_METATYPE(UserConfig)
