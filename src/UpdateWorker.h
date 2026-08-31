#pragma once

#include "HttpClient.h"
#include "UpdateService.h"
#include "UserConfig.h"
#include <QObject>

// runs UpdateService::Run on a dedicated thread so its blocking network calls never stall nickel's GUI thread
class UpdateWorker : public QObject
{
    Q_OBJECT

public slots:
    void Run(UserConfig config);
    void Cancel();

signals:
    void Finished(UpdateService::Result result);
    // emitted from the worker thread; queued across to the GUI thread by Qt's auto-connection, since ShowToast is not thread-safe
    void ToastRequested(QString primary, QString secondary, int milliseconds);

private:
    HttpClient Client;
};
