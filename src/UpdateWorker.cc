#include "UpdateWorker.h"

void UpdateWorker::Run(UserConfig config)
{
    Client.BeginSession();
    const auto requestToast = [this](const QString& primary, const QString& secondary, int milliseconds) { emit ToastRequested(primary, secondary, milliseconds); };
    emit Finished(UpdateService::Run(config, Client, requestToast));
}

void UpdateWorker::Cancel()
{
    Client.CancelSession();
}
