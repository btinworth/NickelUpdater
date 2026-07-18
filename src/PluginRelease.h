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
