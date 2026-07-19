#pragma once

#include <QString>

struct PluginRelease
{
    QString TagName;
    QString KoboRootUrl;

    bool IsValid() const
    {
        return !KoboRootUrl.isEmpty();
    }
};
