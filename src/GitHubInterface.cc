#include "GitHubInterface.h"
#include <NickelHook.h>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QUrl>

bool GitHubInterface::HttpGet(const QString& url, QByteArray* output, const QByteArray& acceptHeader)
{
    static QNetworkAccessManager manager;

    QUrl currentUrl = QUrl(url);
    for (int redirectsRemaining = 5; redirectsRemaining > 0; --redirectsRemaining)
    {
        QNetworkRequest request(currentUrl);
        request.setRawHeader("User-Agent", "NickelUpdater");
        request.setRawHeader("Accept", acceptHeader);

        QScopedPointer<QNetworkReply> reply(manager.get(request));
        QEventLoop loop;
        QObject::connect(reply.data(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError)
        {
            nh_log("HTTP GET failed for %s: %s", qPrintable(currentUrl.toString()), qPrintable(reply->errorString()));
            return false;
        }

        const auto redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirectTarget.isEmpty())
        {
            const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode < 200 || statusCode >= 300)
            {
                nh_log("HTTP GET returned status %d for %s", statusCode, qPrintable(currentUrl.toString()));
                return false;
            }

            if (output != nullptr)
            {
                *output = reply->readAll();
            }

            return true;
        }

        currentUrl = currentUrl.resolved(redirectTarget);
    }

    nh_log("Too many redirects for %s", qPrintable(url));
    return false;
}

PluginRelease GitHubInterface::GetLatestRelease(const QString& pluginId)
{
    const auto url = QString("https://api.github.com/repos/%1/releases/latest").arg(pluginId);

    QByteArray output;
    if (!HttpGet(url, &output))
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
    if (!HttpGet(url, &output, "application/vnd.github.sha"))
    {
        return {};
    }

    return QString::fromUtf8(output).trimmed();
}
