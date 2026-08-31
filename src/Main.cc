#include "Constants.h"
#include "Log.h"
#include "NickelUpdater.h"
#include "Toast.h"
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

    // without this an update could write KoboRoot.tgz while nickel hands the partition to the host
    auto* pwm = PlugWorkflowManagerInstance();
    if (pwm != nullptr)
    {
        QObject::connect(pwm, SIGNAL(aboutToConnect()), &nickelUpdater, SLOT(OnUsbConnecting()), Qt::UniqueConnection);
        QObject::connect(pwm, SIGNAL(doneProcessing()), &nickelUpdater, SLOT(OnUsbDoneProcessing()), Qt::UniqueConnection);
    }
    else
    {
        Log("Could not get PlugWorkflowManager instance, updates will not pause for USB");
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
    {
        .name = "_ZN19PlugWorkflowManager14sharedInstanceEv",
        .out = nh_symoutptr(PlugWorkflowManagerInstance),
        .desc = "PlugWorkflowManager::sharedInstance",
    },
    {
        .name = "_ZN20MainWindowController14sharedInstanceEv",
        .out = nh_symoutptr(MainWindowControllerInstance),
        .desc = "MainWindowController::sharedInstance",
        .optional = true,
    },
    {
        .name = "_ZN20MainWindowController5toastERK7QStringS2_i",
        .out = nh_symoutptr(MainWindowControllerToast),
        .desc = "MainWindowController::toast",
        .optional = true,
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
