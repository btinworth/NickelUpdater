#include "UserConfig.h"
#include <QFile>
#include <QSaveFile>

bool UserConfig::Load(const QString& path)
{
    Plugins.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    bool inPluginsSection = false;
    while (!file.atEnd())
    {
        const auto rawLine = QString::fromUtf8(file.readLine());
        const auto line = StripComment(rawLine).trimmed();
        if (line.isEmpty())
        {
            continue;
        }

        if (line.startsWith('[') && line.endsWith(']'))
        {
            const auto section = line.mid(1, line.size() - 2).trimmed().toLower();
            inPluginsSection = (section == "plugins");
            continue;
        }

        if (!inPluginsSection)
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
    file.write("[plugins]\n");
    for (const auto& plugin : Plugins)
    {
        const auto line = QString("%1 = %2\n").arg(plugin.pluginId, plugin.installedVersion);
        file.write(line.toUtf8());
    }

    return file.commit();
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
