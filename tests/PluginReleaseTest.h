#pragma once

#include <QObject>

class PluginReleaseTest : public QObject
{
    Q_OBJECT

private slots:
    void isValidReturnsFalseWhenUrlEmpty();
    void isValidReturnsTrueWhenUrlPresent();
    void tagNameCanBeEmptyWhileValid();
};
