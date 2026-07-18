#include "Utilities.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include "UserConfig.h"
#include <NickelHook.h>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

QString Utilities::ExtractSha256Digest(const QString& digest)
{
    const int colon = digest.indexOf(':');
    if (colon < 0)
    {
        return {};
    }

    return digest.mid(colon + 1).trimmed();
}

bool Utilities::DownloadFile(const QString& url, const QString& outputPath)
{
    QProcess curl;
    QStringList args;
    args << "-fsSL"
         << "-L"
         << "-H" << "User-Agent: NickelUpdater"
         << "-H" << "Accept: application/vnd.github+json"
         << "-o" << outputPath
         << url;
    curl.start("curl", args);
    return curl.waitForFinished() && curl.exitStatus() == QProcess::NormalExit && curl.exitCode() == 0;
}

bool Utilities::VerifySha256(const QString& filePath, const QString& expectedHex)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
    {
        hash.addData(file.read(64 * 1024));
    }

    return QString::fromLatin1(hash.result().toHex()) == expectedHex.toLower();
}

QString Utilities::StageDirectoryForPlugin(const QString& pluginId)
{
    return QDir(CONFIG_DIR).filePath(QString("staging/%1").arg(pluginId));
}

QString Utilities::MergedArchivePath()
{
    return QDir(CONFIG_DIR).filePath("staging/KoboRoot.merged.tgz");
}

bool Utilities::ExtractArchive(const QString& archivePath, const QString& outputDir)
{
    QProcess tar;
    QStringList args;
    args << "-xzf"
         << archivePath
         << "-C"
         << outputDir;
    tar.start("tar", args);
    return tar.waitForFinished() && tar.exitStatus() == QProcess::NormalExit && tar.exitCode() == 0;
}

bool Utilities::CreateArchive(const QString& sourceDir, const QString& archivePath)
{
    QFile::remove(archivePath);

    QProcess tar;
    QStringList args;
    args << "-czf"
         << archivePath
         << "-C"
         << sourceDir
         << ".";
    tar.start("tar", args);
    return tar.waitForFinished() && tar.exitStatus() == QProcess::NormalExit && tar.exitCode() == 0;
}

bool Utilities::PublishArchive(const QString& archivePath)
{
    const auto onboardFilePath = QDir(ONBOARD_DIR).filePath("KoboRoot.tgz");
    QFile::remove(onboardFilePath);
    return QFile::copy(archivePath, onboardFilePath);
}

bool Utilities::RebootDevice()
{
    return QProcess::startDetached("reboot");
}

bool Utilities::EnsureMergeDirectoryReady(const QString& mergeDirPath)
{
    const auto stagingRootPath = QFileInfo(mergeDirPath).dir().absolutePath();
    QDir stagingRoot(stagingRootPath);
    if (stagingRoot.exists() && !stagingRoot.removeRecursively())
    {
        nh_log("NickelUpdater: failed to clear staging directory: %s", qPrintable(stagingRootPath));
        return false;
    }

    if (!QDir().mkpath(mergeDirPath))
    {
        nh_log("NickelUpdater: failed to create merge directory: %s", qPrintable(mergeDirPath));
        return false;
    }

    return true;
}

QString Utilities::ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath)
{
    const auto release = GitHubInterface::GetLatestRelease(plugin.PluginId);
    if (!release.IsValid())
    {
        nh_log("NickelUpdater: failed to load latest release for %s", qPrintable(plugin.PluginId));
        return {};
    }

    if (!plugin.TagName.isEmpty() && plugin.TagName == release.TagName)
    {
        nh_log("NickelUpdater: plugin %s already at %s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));
        return {};
    }

    nh_log("NickelUpdater: plugin %s installed=%s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));

    const auto stageDirPath = StageDirectoryForPlugin(plugin.PluginId);
    if (!QDir().mkpath(stageDirPath))
    {
        nh_log("NickelUpdater: failed to create stage dir for %s", qPrintable(plugin.PluginId));
        return {};
    }

    const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
    if (!DownloadFile(release.KoboRootUrl, stageFilePath))
    {
        nh_log("NickelUpdater: failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    const auto expectedDigest = ExtractSha256Digest(release.Checksum);
    if (expectedDigest.isEmpty() || !VerifySha256(stageFilePath, expectedDigest))
    {
        nh_log("NickelUpdater: checksum mismatch for %s", qPrintable(plugin.PluginId));
        QFile::remove(stageFilePath);
        return {};
    }

    if (!ExtractArchive(stageFilePath, mergeDirPath))
    {
        nh_log("NickelUpdater: failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    nh_log("NickelUpdater: selected release %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
    nh_log("NickelUpdater: staged KoboRoot.tgz at %s", qPrintable(stageFilePath));
    nh_log("NickelUpdater: KoboRoot.tgz checksum %s", qPrintable(release.Checksum));

    return release.TagName;
}

bool Utilities::FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath)
{
    if (!config.Save(NICKELUPDATER_CONF))
    {
        nh_log("NickelUpdater: failed to save updated tags");
        return false;
    }

    const auto mergedArchivePath = MergedArchivePath();
    if (!CreateArchive(mergeDirPath, mergedArchivePath))
    {
        nh_log("NickelUpdater: failed to create merged KoboRoot.tgz");
        return false;
    }

    if (!PublishArchive(mergedArchivePath))
    {
        nh_log("NickelUpdater: failed to publish merged KoboRoot.tgz");
        return false;
    }

    if (!RebootDevice())
    {
        nh_log("NickelUpdater: failed to reboot after publishing merged KoboRoot.tgz");
        return false;
    }

    nh_log("NickelUpdater: published merged KoboRoot.tgz");
    return true;
}

bool Utilities::PrepareMergeDirectory(const QString& mergeDirPath)
{
    return EnsureMergeDirectoryReady(mergeDirPath);
}

void Utilities::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        nh_log("NickelUpdater: failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return; // nothing to do
    }

    if (QFile::copy(tmplFilePath, filePath))
    {
        nh_log("NickelUpdater: created config from template: %s", filePath);
    }
    else
    {
        nh_log("NickelUpdater: failed to create config from template: %s -> %s", tmplFilePath, filePath);
    }
}

QString Utilities::MergeDirectoryPath()
{
    return QDir(CONFIG_DIR).filePath("staging/_merged_root");
}
