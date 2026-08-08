#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

class Utilities
{
public:
    static void CreateConfig(const char* filePath, const char* tmplFilePath);

    static bool RunProcess(const QString& program, const QStringList& args, QByteArray* output = nullptr);

    static bool HttpGet(const QString& url, QByteArray* output, const QByteArray& acceptHeader = "application/vnd.github+json");
};
