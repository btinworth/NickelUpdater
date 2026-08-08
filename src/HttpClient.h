#pragma once

#include <QByteArray>
#include <QString>

class HttpClient
{
public:
    static bool Get(const QString& url, QByteArray* output, const QByteArray& acceptHeader = "application/vnd.github+json");
};
