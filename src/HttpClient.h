#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class HttpClient
{
public:
    void BeginSession();
    void CancelSession();
    bool Get(const QString& url, QByteArray* output, const QByteArray& acceptHeader = "application/vnd.github+json");
    bool IsRateLimited() const;

private:
    bool RequestSessionCanceled = false;
    bool RateLimited = false;
    QNetworkReply* ActiveReply = nullptr;
    QNetworkAccessManager Manager;
};
