#pragma once

#include "HttpClient.h"
#include "PluginRelease.h"

class GitHubInterface
{
public:
    static PluginRelease GetLatestRelease(HttpClient& httpClient, const QString& pluginId);

private:
    static QString GetCommitHash(HttpClient& httpClient, const QString& pluginId, const QString& tagName);
};
