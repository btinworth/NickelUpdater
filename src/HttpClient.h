#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QString>

class QNetworkReply;

class HttpClient
{
public:
    void BeginSession();
    void CancelSession();
    bool Get(const QString& url, QByteArray* output, const QByteArray& acceptHeader = "application/vnd.github+json");

private:
    bool RequestSessionCanceled = false;
    QNetworkReply* ActiveReply = nullptr;
    QNetworkAccessManager Manager;
};
