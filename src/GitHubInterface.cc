#include "GitHubInterface.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace
{
bool IsTrustedDownloadUrl(const QString& url)
{
    const QUrl parsed(url);
    if (parsed.scheme() != "https")
    {
        return false;
    }

    const auto host = parsed.host();
    return host == "github.com" || host.endsWith(".github.com") || host == "githubusercontent.com" || host.endsWith(".githubusercontent.com");
}
}

PluginRelease GitHubInterface::GetLatestRelease(HttpClient& httpClient, const QString& pluginId)
{
    const auto url = QString("https://api.github.com/repos/%1/releases/latest").arg(pluginId);

    QByteArray output;
    if (!httpClient.Get(url, &output))
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

        const auto downloadUrl = assetObject.value("browser_download_url").toString();
        if (!IsTrustedDownloadUrl(downloadUrl))
        {
            return {};
        }

        const auto commitHash = GetCommitHash(httpClient, pluginId, tagName);
        if (commitHash.isEmpty())
        {
            return {};
        }

        PluginRelease release;
        release.KoboRootUrl = downloadUrl;
        release.TagName = QString("%1@%2").arg(tagName, commitHash);
        release.Size = static_cast<qint64>(assetObject.value("size").toDouble());

        // "digest" is only present for assets uploaded after GitHub added checksums; format is "sha256:<hex>"
        const auto digest = assetObject.value("digest").toString();
        if (digest.startsWith("sha256:"))
        {
            release.Sha256Digest = digest.mid(7);
        }

        return release;
    }

    return {};
}

QString GitHubInterface::GetCommitHash(HttpClient& httpClient, const QString& pluginId, const QString& tagName)
{
    // tagName comes from the GitHub API response, not just from a trusted literal, so it must be encoded before use in a URL path segment
    const auto encodedTagName = QString::fromUtf8(QUrl::toPercentEncoding(tagName));
    const auto url = QString("https://api.github.com/repos/%1/commits/%2").arg(pluginId, encodedTagName);

    QByteArray output;
    if (!httpClient.Get(url, &output, "application/vnd.github.sha"))
    {
        return {};
    }

    return QString::fromUtf8(output).trimmed();
}
