#include "GitHubInterfaceTest.h"
#include "GitHubInterface.h"
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QtTest>

void GitHubInterfaceTest::init()
{
    TempRoot = QDir::temp().filePath(
        QString("nickelupdater-github-test-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    BinDir = QDir(TempRoot).filePath("bin");
    QVERIFY(QDir().mkpath(BinDir));

    OriginalPath = qgetenv("PATH");
    qputenv("PATH", BinDir.toUtf8() + ":" + OriginalPath);
}

void GitHubInterfaceTest::cleanup()
{
    qputenv("PATH", OriginalPath);
    QDir(TempRoot).removeRecursively();
}

void GitHubInterfaceTest::WriteFakeWgetScript(int exitCode, const QByteArray& response,
                                               int commitExitCode, const QByteArray& commitResponse) const
{
    // Write the responses to files to avoid any shell-escaping concerns
    const auto responsePath = QDir(TempRoot).filePath("response.json");
    QFile responseFile(responsePath);
    QVERIFY(responseFile.open(QIODevice::WriteOnly));
    responseFile.write(response);
    responseFile.close();

    const auto commitResponsePath = QDir(TempRoot).filePath("commit-response.json");
    QFile commitResponseFile(commitResponsePath);
    QVERIFY(commitResponseFile.open(QIODevice::WriteOnly));
    commitResponseFile.write(commitResponse);
    commitResponseFile.close();

    const auto scriptPath = QDir(BinDir).filePath("wget");
    QFile script(scriptPath);
    QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));
    script.write(QString(
                     "#!/bin/sh\n"
                     "url=\"\"\n"
                     "for arg in \"$@\"; do url=\"$arg\"; done\n"
                     "case \"$url\" in\n"
                     "  *commits*)\n"
                     "    cat '%1'\n"
                     "    exit %2\n"
                     "    ;;\n"
                     "  *)\n"
                     "    cat '%3'\n"
                     "    exit %4\n"
                     "    ;;\n"
                     "esac\n")
                     .arg(commitResponsePath)
                     .arg(commitExitCode)
                     .arg(responsePath)
                     .arg(exitCode)
                     .toUtf8());
    script.close();

    QFile::setPermissions(scriptPath,
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
        QFile::ReadGroup | QFile::ExeGroup |
        QFile::ReadOther | QFile::ExeOther);
}

void GitHubInterfaceTest::returnsInvalidWhenWgetFails()
{
    WriteFakeWgetScript(1, "");
    QVERIFY(!GitHubInterface::GetLatestRelease("owner/repo").IsValid());
}

void GitHubInterfaceTest::returnsInvalidWhenResponseIsNotJson()
{
    WriteFakeWgetScript(0, "not json at all");
    QVERIFY(!GitHubInterface::GetLatestRelease("owner/repo").IsValid());
}

void GitHubInterfaceTest::returnsInvalidWhenTagNameMissing()
{
    WriteFakeWgetScript(0,
        R"({"assets":[{"name":"KoboRoot.tgz","browser_download_url":"http://example.com/KoboRoot.tgz"}]})");
    QVERIFY(!GitHubInterface::GetLatestRelease("owner/repo").IsValid());
}

void GitHubInterfaceTest::returnsInvalidWhenTagNameEmpty()
{
    WriteFakeWgetScript(0,
        R"({"tag_name":"","assets":[{"name":"KoboRoot.tgz","browser_download_url":"http://example.com/KoboRoot.tgz"}]})");
    QVERIFY(!GitHubInterface::GetLatestRelease("owner/repo").IsValid());
}

void GitHubInterfaceTest::returnsInvalidWhenAssetsArrayMissing()
{
    WriteFakeWgetScript(0, R"({"tag_name":"v1.0.0"})");
    QVERIFY(!GitHubInterface::GetLatestRelease("owner/repo").IsValid());
}

void GitHubInterfaceTest::returnsInvalidWhenNoKoboRootAsset()
{
    WriteFakeWgetScript(0,
        R"({"tag_name":"v1.0.0","assets":[{"name":"OtherFile.tgz","browser_download_url":"http://example.com/OtherFile.tgz"}]})");
    QVERIFY(!GitHubInterface::GetLatestRelease("owner/repo").IsValid());
}

void GitHubInterfaceTest::returnsValidReleaseForWellFormedResponse()
{
    WriteFakeWgetScript(0,
        R"({"tag_name":"v1.2.3","assets":[{"name":"KoboRoot.tgz","browser_download_url":"http://example.com/KoboRoot.tgz"}]})",
        0, R"({"sha":"a1b2c3d4e5f6"})");
    const auto release = GitHubInterface::GetLatestRelease("owner/repo");
    QVERIFY(release.IsValid());
    QCOMPARE(release.TagName, QString("v1.2.3@a1b2c3d4e5f6"));
    QCOMPARE(release.KoboRootUrl, QString("http://example.com/KoboRoot.tgz"));
}

void GitHubInterfaceTest::selectsKoboRootUrlWhenMultipleAssets()
{
    WriteFakeWgetScript(0,
        R"({"tag_name":"v2.0.0","assets":[)"
        R"({"name":"checksums.txt","browser_download_url":"http://example.com/checksums.txt"},)"
        R"({"name":"KoboRoot.tgz","browser_download_url":"http://example.com/KoboRoot.tgz"},)"
        R"({"name":"debug.zip","browser_download_url":"http://example.com/debug.zip"})"
        R"(]})",
        0, R"({"sha":"b2c3d4e5f6a1"})");
    const auto release = GitHubInterface::GetLatestRelease("owner/repo");
    QVERIFY(release.IsValid());
    QCOMPARE(release.TagName, QString("v2.0.0@b2c3d4e5f6a1"));
    QCOMPARE(release.KoboRootUrl, QString("http://example.com/KoboRoot.tgz"));
}

void GitHubInterfaceTest::appendsCommitHashToTagNameWhenResolvable()
{
    WriteFakeWgetScript(0,
        R"({"tag_name":"v1.2.3","assets":[{"name":"KoboRoot.tgz","browser_download_url":"http://example.com/KoboRoot.tgz"}]})",
        0, R"({"sha":"a1b2c3d4e5f6"})");
    const auto release = GitHubInterface::GetLatestRelease("owner/repo");
    QVERIFY(release.IsValid());
    QCOMPARE(release.TagName, QString("v1.2.3@a1b2c3d4e5f6"));
}

void GitHubInterfaceTest::returnsInvalidWhenCommitHashCannotBeResolved()
{
    WriteFakeWgetScript(0,
        R"({"tag_name":"v1.2.3","assets":[{"name":"KoboRoot.tgz","browser_download_url":"http://example.com/KoboRoot.tgz"}]})",
        1, "");
    QVERIFY(!GitHubInterface::GetLatestRelease("owner/repo").IsValid());
}
