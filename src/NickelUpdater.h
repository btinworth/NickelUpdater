#pragma once

#include <QObject>
#include <QThread>

class UpdateWorker;

extern QObject* (*WirelessManagerInstance)();

class NickelUpdater : public QObject
{
    Q_OBJECT

public:
    NickelUpdater();
    ~NickelUpdater() override;

public slots:
    void OnNetworkConnected();

signals:
    void UpdateFinished(bool hasUpdates);

private slots:
    void OnUpdateFinished(bool hasUpdates);

private:
    QThread WorkerThread;
    UpdateWorker* Worker;
};
