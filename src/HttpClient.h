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
    bool IsSessionCanceled() const;
    bool Get(const QString& url, QByteArray* output, const QByteArray& acceptHeader = "application/vnd.github+json");

private:
    bool RequestSessionCanceled = false;
    QNetworkReply* ActiveReply = nullptr;
    QNetworkAccessManager Manager;
};
