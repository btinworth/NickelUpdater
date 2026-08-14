#pragma once

#include <QString>

struct PluginRelease
{
    QString TagName;
    QString KoboRootUrl;
    QString AssetDigest;

    bool IsValid() const
    {
        return !KoboRootUrl.isEmpty();
    }
};
