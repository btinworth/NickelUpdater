#pragma once

#include "UpdateService.h"
#include <QObject>
#include <QThread>

extern QObject* (*WirelessManagerInstance)();
extern QObject* (*PlugWorkflowManagerInstance)();

class UpdateWorker;

class NickelUpdater : public QObject
{
    Q_OBJECT

public:
    NickelUpdater();
    ~NickelUpdater() override;

public slots:
    void OnNetworkConnected();
    void OnNetworkDisconnected();
    void OnUsbConnecting();
    void OnUsbDoneProcessing();

signals:
    void RequestUpdate(UserConfig config);
    void RequestCancel();

private slots:
    void OnUpdateFinished(UpdateService::Result result);

private:
    bool IsUpdating;
    bool UsbConnected;
    QThread WorkerThread;
    UpdateWorker* Worker;

    static void CreateConfig(const char* filePath, const char* tmplFilePath);
};
