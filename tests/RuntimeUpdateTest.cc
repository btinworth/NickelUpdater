#include "RuntimeUpdateTest.h"

#include "Constants.h"
#include "NickelUpdater.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QUuid>
#include <QtTest>

void RuntimeUpdateTest::WriteConfig(const QString& contents) const
{
    QFile file(ConfigPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(contents.toUtf8());
    file.close();
}

QString RuntimeUpdateTest::ReadFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QStringList RuntimeUpdateTest::TarList(const QString& archivePath) const
{
    QProcess tar;
    QStringList args;
    args << "-tzf"
         << archivePath;
    tar.start("tar", args);
    if (!tar.waitForFinished() || tar.exitStatus() != QProcess::NormalExit || tar.exitCode() != 0)
    {
        return {};
    }

    const auto output = QString::fromUtf8(tar.readAllStandardOutput());
    return output.split('\n', Qt::SkipEmptyParts);
}

QString RuntimeUpdateTest::Sha256OfFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
    {
        hash.addData(file.read(64 * 1024));
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString RuntimeUpdateTest::CreateArchiveWithFile(const QString& relativePath, const QString& content, const QString& archiveBaseName) const
{
    const auto rootDir = QDir(DataDir).filePath(QString("root-%1").arg(archiveBaseName));
    const auto filePath = QDir(rootDir).filePath(relativePath);
    if (!QDir().mkpath(QFileInfo(filePath).dir().absolutePath()))
    {
        return {};
    }

    QFile payload(filePath);
    if (!payload.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return {};
    }
    payload.write(content.toUtf8());
    payload.close();

    const auto archivePath = QDir(DataDir).filePath(QString("%1.tgz").arg(archiveBaseName));
    QProcess tar;
    QStringList args;
    args << "-czf"
         << archivePath
         << "-C"
         << rootDir
         << ".";
    tar.start("tar", args);
    if (!tar.waitForFinished() || tar.exitStatus() != QProcess::NormalExit || tar.exitCode() != 0)
    {
        return {};
    }

    return archivePath;
}

void RuntimeUpdateTest::WriteFakeCurlScript() const
{
    const auto scriptPath = QDir(BinDir).filePath("curl");
    QFile script(scriptPath);
    QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));
    script.write(
        "#!/bin/sh\n"
        "out=\"\"\n"
        "url=\"\"\n"
        "while [ $# -gt 0 ]; do\n"
        "  if [ \"$1\" = \"-o\" ]; then\n"
        "    out=\"$2\"\n"
        "    shift 2\n"
        "  else\n"
        "    url=\"$1\"\n"
        "    shift\n"
        "  fi\n"
        "done\n"
        "if [ -n \"$out\" ]; then\n"
        "  if [ \"$url\" = \"http://fake/plugin-a.tgz\" ]; then\n"
        "    cp \"$NU_PLUGIN_A_ARCHIVE\" \"$out\"\n"
        "    exit $?\n"
        "  fi\n"
        "  if [ \"$url\" = \"http://fake/plugin-b.tgz\" ]; then\n"
        "    cp \"$NU_PLUGIN_B_ARCHIVE\" \"$out\"\n"
        "    exit $?\n"
        "  fi\n"
        "  exit 1\n"
        "fi\n"
        "if [ \"$url\" = \"https://api.github.com/repos/owner/plugin-a/releases/latest\" ]; then\n"
        "  printf '{\"tag_name\":\"%s\",\"assets\":[{\"name\":\"KoboRoot.tgz\",\"browser_download_url\":\"http://fake/plugin-a.tgz\",\"digest\":\"sha256:%s\"}]}' \"$NU_PLUGIN_A_TAG\" \"$NU_PLUGIN_A_DIGEST\"\n"
        "  exit 0\n"
        "fi\n"
        "if [ \"$url\" = \"https://api.github.com/repos/owner/plugin-b/releases/latest\" ]; then\n"
        "  printf '{\"tag_name\":\"%s\",\"assets\":[{\"name\":\"KoboRoot.tgz\",\"browser_download_url\":\"http://fake/plugin-b.tgz\",\"digest\":\"sha256:%s\"}]}' \"$NU_PLUGIN_B_TAG\" \"$NU_PLUGIN_B_DIGEST\"\n"
        "  exit 0\n"
        "fi\n"
        "exit 1\n");
    script.close();

    QFile::setPermissions(scriptPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                        QFile::ReadGroup | QFile::ExeGroup |
                                        QFile::ReadOther | QFile::ExeOther);
}

void RuntimeUpdateTest::WriteFakeRebootScript() const
{
    const auto scriptPath = QDir(BinDir).filePath("reboot");
    QFile script(scriptPath);
    QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));
    script.write(
        "#!/bin/sh\n"
        "count=0\n"
        "if [ -f \"$NU_REBOOT_COUNT_FILE\" ]; then\n"
        "  count=$(cat \"$NU_REBOOT_COUNT_FILE\")\n"
        "fi\n"
        "count=$((count+1))\n"
        "printf '%s' \"$count\" > \"$NU_REBOOT_COUNT_FILE\"\n"
        "exit 0\n");
    script.close();

    QFile::setPermissions(scriptPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                        QFile::ReadGroup | QFile::ExeGroup |
                                        QFile::ReadOther | QFile::ExeOther);
}

void RuntimeUpdateTest::init()
{
    OriginalOnboardDir = ONBOARD_DIR;
    OriginalConfigDir = CONFIG_DIR;
    OriginalConfigPath = NICKELUPDATER_CONF;
    OriginalTemplatePath = NICKELUPDATER_TMPL;

    TempRoot = QDir::temp().filePath(QString("nickelupdater-runtime-test-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    BinDir = QDir(TempRoot).filePath("bin");
    DataDir = QDir(TempRoot).filePath("data");
    OnboardDir = QDir(TempRoot).filePath("onboard");
    ConfigPath = QDir(TempRoot).filePath("nickelupdater.conf");
    TemplatePath = QDir(TempRoot).filePath("nickelupdater.conf.tmpl");

    QVERIFY(QDir().mkpath(BinDir));
    QVERIFY(QDir().mkpath(DataDir));
    QVERIFY(QDir().mkpath(OnboardDir));
    QVERIFY(QDir().mkpath(TempRoot));

    OnboardDirBytes = OnboardDir.toUtf8();
    ConfigDirBytes = TempRoot.toUtf8();
    ConfigPathBytes = ConfigPath.toUtf8();
    TemplatePathBytes = TemplatePath.toUtf8();

    OriginalPath = qgetenv("PATH");
    const auto testPath = BinDir.toUtf8() + ":" + OriginalPath;
    qputenv("PATH", testPath);

    ONBOARD_DIR = OnboardDirBytes.constData();
    CONFIG_DIR = ConfigDirBytes.constData();
    NICKELUPDATER_CONF = ConfigPathBytes.constData();
    NICKELUPDATER_TMPL = TemplatePathBytes.constData();

    QFile templateFile(TemplatePath);
    QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
    templateFile.write("# template\n");
    templateFile.close();

    WriteFakeCurlScript();
    WriteFakeRebootScript();
}

void RuntimeUpdateTest::cleanup()
{
    ONBOARD_DIR = OriginalOnboardDir;
    CONFIG_DIR = OriginalConfigDir;
    NICKELUPDATER_CONF = OriginalConfigPath;
    NICKELUPDATER_TMPL = OriginalTemplatePath;
    qputenv("PATH", OriginalPath);

    QDir(TempRoot).removeRecursively();
}

void RuntimeUpdateTest::batchesMultipleUpdatesIntoSingleFinalize()
{
    const auto pluginAArchive = CreateArchiveWithFile("plugin-a/a.txt", "A", "plugin-a");
    const auto pluginBArchive = CreateArchiveWithFile("plugin-b/b.txt", "B", "plugin-b");
    QVERIFY(!pluginAArchive.isEmpty());
    QVERIFY(!pluginBArchive.isEmpty());
    qputenv("NU_PLUGIN_A_ARCHIVE", pluginAArchive.toUtf8());
    qputenv("NU_PLUGIN_B_ARCHIVE", pluginBArchive.toUtf8());
    qputenv("NU_PLUGIN_A_TAG", QByteArray("v2"));
    qputenv("NU_PLUGIN_B_TAG", QByteArray("v11"));
    qputenv("NU_PLUGIN_A_DIGEST", Sha256OfFile(pluginAArchive).toUtf8());
    qputenv("NU_PLUGIN_B_DIGEST", Sha256OfFile(pluginBArchive).toUtf8());

    WriteConfig(
        "owner/plugin-a = v1\n"
        "owner/plugin-b = v10\n");

    NickelUpdater updater;
    updater.OnNetworkConnected();

    const auto savedConfig = ReadFile(ConfigPath);
    QVERIFY(savedConfig.contains("owner/plugin-a = v2\n"));
    QVERIFY(savedConfig.contains("owner/plugin-b = v11\n"));

    const auto onboardArchive = QDir(OnboardDir).filePath("KoboRoot.tgz");
    QVERIFY(QFile::exists(onboardArchive));
    const auto members = TarList(onboardArchive);
    QVERIFY(members.contains("./plugin-a/a.txt"));
    QVERIFY(members.contains("./plugin-b/b.txt"));
}

void RuntimeUpdateTest::conflictLikeFailureSkipsOnePluginButAppliesOthers()
{
    const auto pluginAArchive = CreateArchiveWithFile("plugin-a/a.txt", "A", "plugin-a");
    const auto pluginBArchive = CreateArchiveWithFile("plugin-b/b.txt", "B", "plugin-b");
    QVERIFY(!pluginAArchive.isEmpty());
    QVERIFY(!pluginBArchive.isEmpty());
    qputenv("NU_PLUGIN_A_ARCHIVE", pluginAArchive.toUtf8());
    qputenv("NU_PLUGIN_B_ARCHIVE", pluginBArchive.toUtf8());
    qputenv("NU_PLUGIN_A_TAG", QByteArray("v2"));
    qputenv("NU_PLUGIN_B_TAG", QByteArray("v6"));
    qputenv("NU_PLUGIN_A_DIGEST", QByteArray("deadbeef")); // force checksum failure
    qputenv("NU_PLUGIN_B_DIGEST", Sha256OfFile(pluginBArchive).toUtf8());

    WriteConfig(
        "owner/plugin-a = v1\n"
        "owner/plugin-b = v5\n");

    NickelUpdater updater;
    updater.OnNetworkConnected();

    const auto savedConfig = ReadFile(ConfigPath);
    QVERIFY(savedConfig.contains("owner/plugin-a = v1\n"));
    QVERIFY(savedConfig.contains("owner/plugin-b = v6\n"));

    const auto onboardArchive = QDir(OnboardDir).filePath("KoboRoot.tgz");
    QVERIFY(QFile::exists(onboardArchive));
    const auto members = TarList(onboardArchive);
    QVERIFY(!members.contains("./plugin-a/a.txt"));
    QVERIFY(members.contains("./plugin-b/b.txt"));
}

void RuntimeUpdateTest::noEffectiveChangeDoesNotFinalize()
{
    const auto pluginAArchive = CreateArchiveWithFile("plugin-a/a.txt", "A", "plugin-a");
    const auto pluginBArchive = CreateArchiveWithFile("plugin-b/b.txt", "B", "plugin-b");
    QVERIFY(!pluginAArchive.isEmpty());
    QVERIFY(!pluginBArchive.isEmpty());
    qputenv("NU_PLUGIN_A_ARCHIVE", pluginAArchive.toUtf8());
    qputenv("NU_PLUGIN_B_ARCHIVE", pluginBArchive.toUtf8());
    qputenv("NU_PLUGIN_A_TAG", QByteArray("v1"));
    qputenv("NU_PLUGIN_B_TAG", QByteArray("v2"));
    qputenv("NU_PLUGIN_A_DIGEST", Sha256OfFile(pluginAArchive).toUtf8());
    qputenv("NU_PLUGIN_B_DIGEST", Sha256OfFile(pluginBArchive).toUtf8());

    const QString initialConfig =
        "owner/plugin-a = v1\n"
        "owner/plugin-b = v2\n";
    WriteConfig(initialConfig);

    NickelUpdater updater;
    updater.OnNetworkConnected();

    QCOMPARE(ReadFile(ConfigPath), initialConfig);
    QVERIFY(!QFile::exists(QDir(OnboardDir).filePath("KoboRoot.tgz")));
}
