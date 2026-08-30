#include "Constants.h"
#include "Log.h"
#include "NickelUpdater.h"
#include <NickelHook.h>
#include <QDir>

static int NickelUpdaterInit()
{
    SetLogEnabled(true);

    static NickelUpdater nickelUpdater;

    auto* wm = WirelessManagerInstance();
    if (wm != nullptr)
    {
        QObject::connect(wm, SIGNAL(networkConnected()), &nickelUpdater, SLOT(OnNetworkConnected()), Qt::UniqueConnection);
        QObject::connect(wm, SIGNAL(networkDisconnected()), &nickelUpdater, SLOT(OnNetworkDisconnected()), Qt::UniqueConnection);
    }
    else
    {
        Log("Could not get WirelessManager instance");
        return 1;
    }

    return 0;
}

static bool NickelUpdaterUninstall()
{
    Log("Removing NickelUpdater config and program files");

    const char* const dirs[] = {CONFIG_DIR, INSTALL_DIR};

    auto deleted = true;
    for (const auto* path : dirs)
    {
        QDir dir(path);
        deleted &= !dir.exists() || dir.removeRecursively();
    }

    return deleted;
}

static struct nh_info NickelUpdaterInfo = {
    .name = "NickelUpdater",
    .desc = "Auto-update Kobo plugins from GitHub releases",
    .uninstall_flag = UNINSTALL_FLAG,
};

static struct nh_dlsym NickelUpdaterDlsym[] = {
    {
        .name = "_ZN15WirelessManager14sharedInstanceEv",
        .out = nh_symoutptr(WirelessManagerInstance),
        .desc = "WirelessManager::sharedInstance",
    },
    {0},
};

NickelHook(
    .init = NickelUpdaterInit,
    .info = &NickelUpdaterInfo,
    .hook = nullptr,
    .dlsym = NickelUpdaterDlsym,
    .uninstall = NickelUpdaterUninstall,
)
