#include "UpdateWorker.h"

void UpdateWorker::Run(UserConfig config)
{
    Client.BeginSession();
    emit Finished(UpdateService::Run(config, Client));
}

void UpdateWorker::Cancel()
{
    Client.CancelSession();
}
