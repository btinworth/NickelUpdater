#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class HttpClient
{
public:
    HttpClient();
    ~HttpClient();

    void BeginSession();
    void CancelSession();
    bool IsSessionCanceled() const;
    bool Get(const QString& url, QByteArray* output, const QByteArray& acceptHeader = "application/vnd.github+json");

private:
    // created lazily on first use, so it picks up the thread affinity of whichever thread actually makes requests
    QNetworkAccessManager& GetManager();

    bool RequestSessionCanceled = false;
    QNetworkReply* ActiveReply = nullptr;
    QNetworkAccessManager* Manager = nullptr;
};
