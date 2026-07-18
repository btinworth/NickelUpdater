#pragma once

#include "PluginRelease.h"
#include <QObject>

class PluginReleaseTest : public QObject
{
    Q_OBJECT

private slots:
    void selectsKoboRootAssetAndChecksum();
    void returnsInvalidReleaseWithoutKoboRootAsset();
};
