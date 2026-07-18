#include "PluginRelease.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

PluginRelease PluginReleaseClient::GetLatestRelease(const QString& pluginId) const
{
    QProcess curl;
    curl.start("curl", QStringList{
        "-fsSL",
        "-H", "User-Agent: NickelUpdater",
        "-H", "Accept: application/vnd.github+json",
        QString("https://api.github.com/repos/%1/releases/latest").arg(pluginId),
    });
    if (!curl.waitForFinished() || curl.exitStatus() != QProcess::NormalExit || curl.exitCode() != 0)
    {
        return {};
    }

    return ParseRelease(curl.readAllStandardOutput());
}

PluginRelease PluginReleaseClient::ParseRelease(const QByteArray& data)
{
    const auto document = QJsonDocument::fromJson(data);
    if (!document.isObject())
    {
        return {};
    }

    const auto releaseObject = document.object();

    const auto tagName = releaseObject.value("tag_name").toString();
    if (tagName.isEmpty())
    {
        return {};
    }

    const auto assets = releaseObject.value("assets").toArray();
    for (const auto& assetValue : assets)
    {
        if (!assetValue.isObject())
        {
            continue;
        }

        const auto assetObject = assetValue.toObject();
        if (assetObject.value("name").toString() != "KoboRoot.tgz")
        {
            continue;
        }

        PluginRelease release;
        release.KoboRootUrl = assetObject.value("browser_download_url").toString();
        release.Checksum = assetObject.value("digest").toString();
        release.TagName = tagName;
        return release;
    }

    return {};
}
