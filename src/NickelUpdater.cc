#include "NickelUpdater.h"
#include "Constants.h"
#include "Log.h"
#include "Toast.h"
#include "UpdateWorker.h"
#include <QDir>
#include <QFile>

QObject* (*WirelessManagerInstance)() = nullptr;
QObject* (*PlugWorkflowManagerInstance)() = nullptr;

NickelUpdater::NickelUpdater()
{
    qRegisterMetaType<UserConfig>();
    qRegisterMetaType<UpdateService::Result>();

    CreateConfig(NICKELUPDATER_CONF, NICKELUPDATER_TMPL);

    // the worker owns its own HttpClient/QNetworkAccessManager so its blocking network calls never stall this (nickel's GUI) thread
    Worker = new UpdateWorker();
    Worker->moveToThread(&WorkerThread);
    connect(&WorkerThread, &QThread::finished, Worker, &QObject::deleteLater);
    connect(this, &NickelUpdater::RequestUpdate, Worker, &UpdateWorker::Run);
    connect(this, &NickelUpdater::RequestCancel, Worker, &UpdateWorker::Cancel);
    connect(Worker, &UpdateWorker::Finished, this, &NickelUpdater::OnUpdateFinished);
    connect(Worker, &UpdateWorker::ToastRequested, this, &NickelUpdater::OnToastRequested);
    WorkerThread.start();
}

NickelUpdater::~NickelUpdater()
{
    WorkerThread.quit();
    WorkerThread.wait();
}

void NickelUpdater::OnNetworkConnected()
{
    if (UsbConnected)
    {
        Log("USB connected; skipping new network-connected trigger");
        return;
    }

    if (IsUpdating)
    {
        Log("Update already in progress; skipping new network-connected trigger");
        return;
    }

    UserConfig config;
    if (!config.Load(NICKELUPDATER_CONF))
    {
        Log("Failed to open config: %s", NICKELUPDATER_CONF);
        return;
    }

    Log("Config loaded from %s (%lld plugin(s))", NICKELUPDATER_CONF, static_cast<long long>(config.GetPlugins().size()));

    IsUpdating = true;
    Log("Starting update");
    emit RequestUpdate(config);
}

void NickelUpdater::OnUpdateFinished(UpdateService::Result result)
{
    switch (result)
    {
    case UpdateService::Result::Failed:
    case UpdateService::Result::Deferred:
        break;
    case UpdateService::Result::NoUpdates:
        Log("No updates to apply");
        break;
    case UpdateService::Result::Updated:
        Log("Update downloaded; will finish installing on next reboot");
        break;
    }

    IsUpdating = false;
}

void NickelUpdater::OnToastRequested(const QString& primary, const QString& secondary, int milliseconds)
{
    // runs on this (GUI) thread via the queued ToastRequested signal, since MainWindowController belongs to it
    ShowToast(primary, secondary, milliseconds);
}

void NickelUpdater::OnNetworkDisconnected()
{
    if (IsUpdating)
    {
        Log("Network disconnected; canceling active update");
    }

    emit RequestCancel();
}

void NickelUpdater::OnUsbConnecting()
{
    // onboard is about to be handed to the host, so nothing under it stays writable
    UsbConnected = true;

    if (IsUpdating)
    {
        Log("USB connecting; canceling active update");
    }

    emit RequestCancel();
}

void NickelUpdater::OnUsbDoneProcessing()
{
    UsbConnected = false;
}

void NickelUpdater::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        Log("Failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return;
    }

    if (QFile::copy(tmplFilePath, filePath))
    {
        Log("Created config from template: %s", filePath);
    }
    else
    {
        Log("Failed to create config from template: %s -> %s", tmplFilePath, filePath);
    }
}
