#pragma once

#include <QObject>

class SmokeTest : public QObject
{
    Q_OBJECT

private slots:
    void constants_haveExpectedNickelUpdaterPaths();
};
