#pragma once

#include "PluginRelease.h"

class GitHubInterface
{
public:
    static PluginRelease GetLatestRelease(const QString& pluginId);
};
