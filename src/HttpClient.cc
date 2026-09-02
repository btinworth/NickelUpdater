#include "HttpClient.h"
#include "Log.h"
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QSslError>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>

namespace
{
const int HTTP_REQUEST_TIMEOUT_MS = 5 * 60 * 1000;
}

HttpClient::HttpClient()
{
    Log("TLS support: %s (library: %s)", QSslSocket::supportsSsl() ? "yes" : "no", qPrintable(QSslSocket::sslLibraryVersionString()));
}

HttpClient::~HttpClient()
{
    delete Manager;
}

QNetworkAccessManager& HttpClient::GetManager()
{
    if (Manager == nullptr)
    {
        Manager = new QNetworkAccessManager();
    }

    return *Manager;
}

void HttpClient::BeginSession()
{
    RequestSessionCanceled = false;
}

void HttpClient::CancelSession()
{
    RequestSessionCanceled = true;
    if (ActiveReply != nullptr)
    {
        ActiveReply->abort();
    }
}

bool HttpClient::IsSessionCanceled() const
{
    return RequestSessionCanceled;
}

bool HttpClient::Get(const QString& url, QByteArray* output, const QByteArray& acceptHeader)
{
    if (RequestSessionCanceled)
    {
        Log("HTTP GET canceled before request for %s", qPrintable(url));
        return false;
    }

    QUrl currentUrl = QUrl(url);
    const int maxRedirects = 5;
    for (int redirectCount = 0; redirectCount <= maxRedirects; ++redirectCount)
    {
        // check before each hop so cancellation also stops a redirect chain
        if (RequestSessionCanceled)
        {
            Log("HTTP GET canceled before request for %s", qPrintable(currentUrl.toString()));
            return false;
        }

        // keep the request identifiable to the server and ask for the caller's response format
        QNetworkRequest request(currentUrl);
        request.setRawHeader("User-Agent", "NickelUpdater");
        request.setRawHeader("Accept", acceptHeader);

        // the reply is deleted at the end of this iteration; expose it while active so
        // CancelSession() can abort the request from another event-driven callback
        QScopedPointer<QNetworkReply> reply(GetManager().get(request));
        ActiveReply = reply.data();
        QEventLoop loop;
        bool timedOut = false;

        // aborting the reply causes the event loop to finish through its normal path
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        QObject::connect(&timeoutTimer, &QTimer::timeout, [&timedOut, &reply]() {
            timedOut = true;
            if (!reply.isNull())
            {
                reply->abort();
            }
        });

        // a completed reply is the event-loop exit condition for this request
        QObject::connect(reply.data(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(reply.data(), QOverload<const QList<QSslError>&>::of(&QNetworkReply::sslErrors), [&currentUrl](const QList<QSslError>& errors) {
            for (const auto& error : errors)
            {
                Log("TLS error for %s: %s", qPrintable(currentUrl.toString()), qPrintable(error.errorString()));
            }
        });

        // wait synchronously while still processing network and cancellation events
        // restart on progress so the timeout measures inactivity and not total transfer time
        QObject::connect(reply.data(), &QNetworkReply::downloadProgress, [&timeoutTimer]() { timeoutTimer.start(HTTP_REQUEST_TIMEOUT_MS); });
        timeoutTimer.start(HTTP_REQUEST_TIMEOUT_MS);
        loop.exec();
        timeoutTimer.stop();

        // no further cancellation callback should target this completed reply
        ActiveReply = nullptr;

        if (RequestSessionCanceled)
        {
            Log("HTTP GET canceled during request for %s", qPrintable(currentUrl.toString()));
            return false;
        }

        if (timedOut)
        {
            Log("HTTP GET timed out after %d ms for %s", HTTP_REQUEST_TIMEOUT_MS, qPrintable(currentUrl.toString()));
            return false;
        }

        if (reply->error() != QNetworkReply::NoError)
        {
            Log("HTTP GET failed for %s: %s", qPrintable(currentUrl.toString()), qPrintable(reply->errorString()));
            return false;
        }

        // follow redirects explicitly so each destination can be checked for https
        const auto redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirectTarget.isEmpty())
        {
            const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            // network success only means the exchange completed - accept HTTP success codes explicitly
            if (statusCode < 200 || statusCode >= 300)
            {
                Log("HTTP GET returned status %d for %s", statusCode, qPrintable(currentUrl.toString()));
                return false;
            }

            if (output != nullptr)
            {
                // leave output untouched when the caller only needs the success result
                *output = reply->readAll();
            }

            return true;
        }

        // resolve relative location values against the URL that produced the response
        currentUrl = currentUrl.resolved(redirectTarget);
        if (currentUrl.scheme() != "https")
        {
            Log("Refusing redirect to non-https URL: %s", qPrintable(currentUrl.toString()));
            return false;
        }

        if (redirectCount == maxRedirects)
        {
            // stop looping even if the server keeps returning another redirect
            Log("Too many redirects for %s", qPrintable(url));
            return false;
        }
    }

    return false;
}
