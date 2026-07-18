#include "SmokeTest.h"
#include "Constants.h"
#include <QtTest>

void SmokeTest::constants_haveExpectedNickelUpdaterPaths()
{
    QCOMPARE(QString(CONFIG_DIR), QString("/mnt/onboard/.adds/nickelupdater"));
    QCOMPARE(QString(NICKELUPDATER_CONF), QString("/mnt/onboard/.adds/nickelupdater/nickelupdater.conf"));
    QCOMPARE(QString(NICKELUPDATER_TMPL), QString("/usr/local/nickelupdater/nickelupdater.conf.tmpl"));
}
