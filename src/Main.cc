#include "Constants.h"
#include "Log.h"
#include "NickelUpdater.h"
#include <NickelHook.h>
#include <QDir>

static int NickelUpdaterInit()
{
    SetLogEnabled(true);

    auto* wm = WirelessManagerInstance();
    if (wm == nullptr)
    {
        Log("Could not get WirelessManager instance");
        return 0;
    }

    static NickelUpdater nickelUpdater;
    QObject::connect(wm, SIGNAL(networkConnected()), &nickelUpdater, SLOT(OnNetworkConnected()), Qt::UniqueConnection);
    QObject::connect(wm, SIGNAL(networkDisconnected()), &nickelUpdater, SLOT(OnNetworkDisconnected()), Qt::UniqueConnection);
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
