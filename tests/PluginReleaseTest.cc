#include "PluginReleaseTest.h"
#include "PluginRelease.h"
#include <QtTest>

void PluginReleaseTest::isValidReturnsFalseWhenUrlEmpty()
{
    PluginRelease release;
    QVERIFY(!release.IsValid());
}

void PluginReleaseTest::isValidReturnsTrueWhenUrlPresent()
{
    PluginRelease release;
    release.KoboRootUrl = "http://example.com/KoboRoot.tgz";
    QVERIFY(release.IsValid());
}

void PluginReleaseTest::tagNameCanBeEmptyWhileValid()
{
    PluginRelease release;
    release.KoboRootUrl = "http://example.com/KoboRoot.tgz";
    QVERIFY(release.TagName.isEmpty());
    QVERIFY(release.IsValid());
}
