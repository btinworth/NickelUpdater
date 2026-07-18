#include "PluginReleaseTest.h"
#include <QtTest>

void PluginReleaseTest::selectsKoboRootAssetAndChecksum()
{
    const QByteArray json = R"JSON(
        {
          "tag_name": "v2.0.0",
          "assets": [
            { "name": "ignored.bin", "browser_download_url": "https://example.invalid/ignored.bin", "digest": "sha256:ignored" },
            { "name": "KoboRoot.tgz", "browser_download_url": "https://example.invalid/v2/KoboRoot.tgz", "digest": "sha256:abc123" }
          ]
        }
    )JSON";

    const auto release = PluginReleaseClient::ParseRelease(json);
    QVERIFY(release.IsValid());
    QCOMPARE(release.TagName, QString("v2.0.0"));
    QCOMPARE(release.KoboRootUrl, QString("https://example.invalid/v2/KoboRoot.tgz"));
    QCOMPARE(release.Checksum, QString("sha256:abc123"));
}

void PluginReleaseTest::returnsInvalidReleaseWithoutKoboRootAsset()
{
    const QByteArray json = R"JSON(
        {
          "tag_name": "v3.0.0",
          "assets": [
            { "name": "other.bin", "browser_download_url": "https://example.invalid/other.bin", "digest": "sha256:def456" }
          ]
        }
    )JSON";

    const auto release = PluginReleaseClient::ParseRelease(json);
    QVERIFY(!release.IsValid());
    QVERIFY(release.TagName.isEmpty());
    QVERIFY(release.KoboRootUrl.isEmpty());
    QVERIFY(release.Checksum.isEmpty());
}
