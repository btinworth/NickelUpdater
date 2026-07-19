#include "Constants.h"
#include "NickelUpdater.h"
#include <NickelHook.h>

static int NickelUpdaterInit()
{
    auto* wm = WirelessManagerInstance();
    if (wm == nullptr)
    {
        nh_log("Could not get WirelessManager instance");
        return 0;
    }

    static NickelUpdater nickelUpdater;
    QObject::connect(wm, SIGNAL(networkConnected()), &nickelUpdater, SLOT(OnNetworkConnected()), Qt::UniqueConnection);
    return 0;
}

static struct nh_info NickelUpdaterInfo = {
    .name = "NickelUpdater",
    .desc = "Auto-update Kobo plugins from GitHub releases",
    .uninstall_flag = UNINSTALL_FLAG,
};

static struct nh_hook NickelUpdaterHook[] = {
    {0},
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
    .hook = NickelUpdaterHook,
    .dlsym = NickelUpdaterDlsym,
)
