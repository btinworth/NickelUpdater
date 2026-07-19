#include "GitHubInterface.h"
#include "Utilities.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

PluginRelease GitHubInterface::GetLatestRelease(const QString& pluginId)
{
    QByteArray output;
    const bool ok = Utilities::RunProcess("curl",
                                           {
                                               "-fsSL",
                                               "-H", "User-Agent: NickelUpdater",
                                               "-H", "Accept: application/vnd.github+json",
                                               QString("https://api.github.com/repos/%1/releases/latest").arg(pluginId),
                                           },
                                           &output);
    if (!ok)
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

        PluginRelease release;
        release.KoboRootUrl = assetObject.value("browser_download_url").toString();
        release.Checksum = assetObject.value("digest").toString();
        release.TagName = tagName;
        return release;
    }

    return {};
}
