#include "GitHubInterface.h"
#include "Utilities.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

PluginRelease GitHubInterface::GetLatestRelease(const QString& pluginId)
{
    const auto url = QString("https://api.github.com/repos/%1/releases/latest").arg(pluginId);

    QByteArray output;
    if (!Utilities::HttpGet(url, &output))
    {
        return {};
    }

    const auto document = QJsonDocument::fromJson(output);
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

        const auto commitHash = GetCommitHash(pluginId, tagName);
        if (commitHash.isEmpty())
        {
            return {};
        }

        PluginRelease release;
        release.KoboRootUrl = assetObject.value("browser_download_url").toString();
        release.TagName = QString("%1@%2").arg(tagName, commitHash);
        return release;
    }

    return {};
}

QString GitHubInterface::GetCommitHash(const QString& pluginId, const QString& tagName)
{
    const auto url = QString("https://api.github.com/repos/%1/commits/%2").arg(pluginId, tagName);

    QByteArray output;
    if (!Utilities::HttpGet(url, &output, "application/vnd.github.sha"))
    {
        return {};
    }

    return QString::fromUtf8(output).trimmed();
}
