#include "UserConfig.h"
#include "Log.h"
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>

namespace
{
// GitHub owner/repo names only allow alphanumerics, '.', '-', and '_'
const QRegularExpression PluginIdPattern("^[A-Za-z0-9._-]+/[A-Za-z0-9._-]+$");
}

bool UserConfig::Load(const QString& path)
{
    Plugins.clear();
    QSet<QString> seenPluginIds;

    QFile file(path);
    if (!file.exists())
    {
        Log("Config file does not exist: %s", qPrintable(path));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        Log("Failed to open config file %s: %s", qPrintable(path), qPrintable(file.errorString()));
        return false;
    }

    while (!file.atEnd())
    {
        const auto rawLine = QString::fromUtf8(file.readLine());
        const auto line = StripComment(rawLine).trimmed();
        if (line.isEmpty())
        {
            continue;
        }

        const int equals = line.indexOf('=');
        if (equals < 0)
        {
            continue;
        }

        const auto pluginId = line.left(equals).trimmed();
        const auto installedVersion = line.mid(equals + 1).trimmed();

        if (!PluginIdPattern.match(pluginId).hasMatch())
        {
            Log("Ignoring invalid plugin id in config: %s", qPrintable(pluginId));
            continue;
        }

        if (seenPluginIds.contains(pluginId))
        {
            continue;
        }

        seenPluginIds.insert(pluginId);

        Plugins.push_back({pluginId, installedVersion});
    }

    return true;
}

bool UserConfig::Save(const QString& path) const
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    file.write("# NickelUpdater configuration\n");
    for (const auto& plugin : Plugins)
    {
        const auto line = QString("%1 = %2\n").arg(plugin.PluginId, plugin.TagName);
        file.write(line.toUtf8());
    }

    return file.commit();
}

void UserConfig::SetTag(const QString& pluginId, const QString& tagName)
{
    for (auto& plugin : Plugins)
    {
        if (plugin.PluginId == pluginId)
        {
            plugin.TagName = tagName;
            return;
        }
    }
}

const QVector<PluginConfigEntry>& UserConfig::GetPlugins() const
{
    return Plugins;
}

QString UserConfig::StripComment(const QString& line)
{
    const int comment = line.indexOf('#');
    return comment < 0 ? line : line.left(comment);
}
