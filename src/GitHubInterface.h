#pragma once

#include "PluginRelease.h"
#include <QByteArray>

class GitHubInterface
{
public:
    static PluginRelease GetLatestRelease(const QString& pluginId);

private:
    static bool HttpGet(const QString& url, QByteArray* output, const QByteArray& acceptHeader = "application/vnd.github+json");
    static QString GetCommitHash(const QString& pluginId, const QString& tagName);
};
