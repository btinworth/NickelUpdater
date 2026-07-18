#pragma once

#include <QByteArray>
#include <QString>

struct PluginRelease
{
    QString TagName;
    QString KoboRootUrl;
    QString Checksum;

    bool IsValid() const
    {
        return !KoboRootUrl.isEmpty() && !Checksum.isEmpty();
    }
};

class PluginReleaseClient
{
public:
    PluginRelease GetLatestRelease(const QString& pluginId) const;
    static PluginRelease ParseRelease(const QByteArray& data);
};
