#include "NickelUpdater.h"
#include "Constants.h"
#include "UpdateWorker.h"
#include "Utilities.h"
#include <NickelHook.h>

QObject* (*WirelessManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater() : Worker(new UpdateWorker)
{
    Utilities::CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);

    Worker->moveToThread(&WorkerThread);
    connect(&WorkerThread, &QThread::finished, Worker, &QObject::deleteLater);
    connect(Worker, &UpdateWorker::Finished, this, &NickelUpdater::OnUpdateFinished);
    WorkerThread.start();
}

NickelUpdater::~NickelUpdater()
{
    WorkerThread.quit();
    WorkerThread.wait();
}

void NickelUpdater::OnNetworkConnected()
{
    QMetaObject::invokeMethod(Worker, "Run", Qt::QueuedConnection);
}

void NickelUpdater::OnUpdateFinished(bool hasUpdates)
{
    if (hasUpdates)
    {
        nh_log("Updates staged and published");
    }

    emit UpdateFinished(hasUpdates);
}

