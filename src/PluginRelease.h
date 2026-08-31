#pragma once

#include <QString>

struct PluginRelease
{
    QString TagName;
    QString KoboRootUrl;
    qint64 Size = 0;
    QString Sha256Digest;

    bool IsValid() const
    {
        return !KoboRootUrl.isEmpty();
    }
};
