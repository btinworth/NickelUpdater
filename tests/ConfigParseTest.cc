#include "ConfigParseTest.h"
#include <QFile>
#include <QTemporaryFile>
#include <QtTest>

UserConfig ConfigParseTest::LoadConfig(const QString& contents)
{
    QTemporaryFile file;
    if (!file.open())
    {
        qFatal("failed to create temp config file");
    }

    file.write(contents.toUtf8());
    file.close();

    UserConfig config;
    if (!config.Load(file.fileName()))
    {
        qFatal("failed to load temp config file");
    }

    return config;
}

void ConfigParseTest::parsesValidPluginRows()
{
    const auto config = LoadConfig(
        "pgaskin/NickelMenu = v1.0.0\n"
        "bobo/example-plugin =\n");

    const auto& plugins = config.GetPlugins();
    QCOMPARE(plugins.size(), 2);
    QCOMPARE(plugins.at(0).PluginId, QString("pgaskin/NickelMenu"));
    QCOMPARE(plugins.at(0).TagName, QString("v1.0.0"));
    QCOMPARE(plugins.at(1).PluginId, QString("bobo/example-plugin"));
    QCOMPARE(plugins.at(1).TagName, QString(""));
}

void ConfigParseTest::acceptsArbitraryPluginIds()
{
    const auto config = LoadConfig(
        "missingequals\n"
        "bad/format/row = v1\n"
        "good/repo = v2\n"
        "bad owner/repo = v3\n");

    const auto& plugins = config.GetPlugins();
    QCOMPARE(plugins.size(), 3);
    QCOMPARE(plugins.at(0).PluginId, QString("bad/format/row"));
    QCOMPARE(plugins.at(0).TagName, QString("v1"));
    QCOMPARE(plugins.at(1).PluginId, QString("good/repo"));
    QCOMPARE(plugins.at(1).TagName, QString("v2"));
    QCOMPARE(plugins.at(2).PluginId, QString("bad owner/repo"));
    QCOMPARE(plugins.at(2).TagName, QString("v3"));
}

void ConfigParseTest::saveWritesDeterministicFormat()
{
    QTemporaryFile out;
    if (!out.open())
    {
        qFatal("failed to create temp output file");
    }
    out.close();

    const auto config = LoadConfig(
        "zeta/second = v2\n"
        "alpha/first = v1\n");

    QVERIFY(config.Save(out.fileName()));

    QFile saved(out.fileName());
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto contents = QString::fromUtf8(saved.readAll());

    QVERIFY(contents.startsWith("# NickelUpdater configuration\n"));
    QVERIFY(!contents.contains("[plugins]"));
    QVERIFY(contents.contains("zeta/second = v2\n"));
    QVERIFY(contents.contains("alpha/first = v1\n"));
}

void ConfigParseTest::updatesInstalledVersion()
{
    auto config = LoadConfig(
        "pgaskin/NickelMenu = v1.0.0\n"
        "bobo/example-plugin =\n");

    QVERIFY(config.SetTag("bobo/example-plugin", "v2.0.0"));
    const auto& plugins = config.GetPlugins();
    QCOMPARE(plugins.at(0).TagName, QString("v1.0.0"));
    QCOMPARE(plugins.at(1).TagName, QString("v2.0.0"));
    QVERIFY(!config.SetTag("missing/repo", "v1"));
}
