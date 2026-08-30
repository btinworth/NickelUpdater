#pragma once

#include "HttpClient.h"
#include <QObject>

extern QObject* (*WirelessManagerInstance)();
extern QObject* (*PlugWorkflowManagerInstance)();

class NickelUpdater : public QObject
{
    Q_OBJECT

public:
    NickelUpdater();

public slots:
    void OnNetworkConnected();
    void OnNetworkDisconnected();
    void OnUsbConnecting();
    void OnUsbDoneProcessing();

private:
    HttpClient Client;
    bool IsUpdating;
    bool UsbConnected;

    static void CreateConfig(const char* filePath, const char* tmplFilePath);
};
