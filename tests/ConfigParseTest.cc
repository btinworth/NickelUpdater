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
        "[plugins]\n"
        "pgaskin/NickelMenu = v1.0.0\n"
        "bobo/example-plugin =\n");

    const auto& plugins = config.GetPlugins();
    QCOMPARE(plugins.size(), 2);
    QCOMPARE(plugins.at(0).pluginId, QString("pgaskin/NickelMenu"));
    QCOMPARE(plugins.at(0).installedVersion, QString("v1.0.0"));
    QCOMPARE(plugins.at(1).pluginId, QString("bobo/example-plugin"));
    QCOMPARE(plugins.at(1).installedVersion, QString(""));
}

void ConfigParseTest::acceptsArbitraryPluginIds()
{
    const auto config = LoadConfig(
        "[plugins]\n"
        "missingequals\n"
        "bad/format/row = v1\n"
        "good/repo = v2\n"
        "bad owner/repo = v3\n");

    const auto& plugins = config.GetPlugins();
    QCOMPARE(plugins.size(), 3);
    QCOMPARE(plugins.at(0).pluginId, QString("bad/format/row"));
    QCOMPARE(plugins.at(0).installedVersion, QString("v1"));
    QCOMPARE(plugins.at(1).pluginId, QString("good/repo"));
    QCOMPARE(plugins.at(1).installedVersion, QString("v2"));
    QCOMPARE(plugins.at(2).pluginId, QString("bad owner/repo"));
    QCOMPARE(plugins.at(2).installedVersion, QString("v3"));
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
        "[plugins]\n"
        "zeta/second = v2\n"
        "alpha/first = v1\n");

    QVERIFY(config.Save(out.fileName()));

    QFile saved(out.fileName());
    QVERIFY(saved.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto contents = QString::fromUtf8(saved.readAll());

    QVERIFY(contents.startsWith("# NickelUpdater configuration\n[plugins]\n"));
    QVERIFY(contents.contains("zeta/second = v2\n"));
    QVERIFY(contents.contains("alpha/first = v1\n"));
}
