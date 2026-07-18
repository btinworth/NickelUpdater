#pragma once

#include <QObject>

extern QObject* (*WirelessManagerInstance)();

class NickelUpdater : public QObject
{
    Q_OBJECT

public:
    NickelUpdater();

public slots:
    void OnNetworkConnected();
    void OnNetworkDisconnected();

private:
    static void CreateConfig(const char* filePath, const char* tmplFilePath);
};
