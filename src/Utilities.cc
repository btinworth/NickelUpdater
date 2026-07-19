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

bool Utilities::RunProcess(const QString& program, const QStringList& args, QByteArray* output)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForFinished() || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        return false;
    }

    if (output != nullptr)
    {
        *output = process.readAllStandardOutput();
    }

    return true;
}

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
    return RunProcess("curl",
                       {
                           "-fsSL",
                           "-L",
                           "-H", "User-Agent: NickelUpdater",
                           "-H", "Accept: application/vnd.github+json",
                           "-o", outputPath,
                           url,
                       });
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
    return RunProcess("tar", {"-xzf", archivePath, "-C", outputDir});
}

bool Utilities::CreateArchive(const QString& sourceDir, const QString& archivePath)
{
    QFile::remove(archivePath);
    return RunProcess("tar", {"-czf", archivePath, "-C", sourceDir, "."});
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

bool Utilities::PrepareMergeDirectory(const QString& mergeDirPath)
{
    const auto stagingRootPath = QFileInfo(mergeDirPath).dir().absolutePath();
    QDir stagingRoot(stagingRootPath);
    if (stagingRoot.exists() && !stagingRoot.removeRecursively())
    {
        nh_log("Failed to clear staging directory: %s", qPrintable(stagingRootPath));
        return false;
    }

    if (!QDir().mkpath(mergeDirPath))
    {
        nh_log("Failed to create merge directory: %s", qPrintable(mergeDirPath));
        return false;
    }

    return true;
}

QString Utilities::ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath)
{
    const auto release = GitHubInterface::GetLatestRelease(plugin.PluginId);
    if (!release.IsValid())
    {
        nh_log("Failed to load latest release for %s", qPrintable(plugin.PluginId));
        return {};
    }

    if (!plugin.TagName.isEmpty() && plugin.TagName == release.TagName)
    {
        nh_log("Plugin %s already at %s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));
        return {};
    }

    const auto stageDirPath = StageDirectoryForPlugin(plugin.PluginId);
    if (!QDir().mkpath(stageDirPath))
    {
        nh_log("Failed to create stage dir for %s", qPrintable(plugin.PluginId));
        return {};
    }

    const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
    if (!DownloadFile(release.KoboRootUrl, stageFilePath))
    {
        nh_log("Failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    const auto expectedDigest = ExtractSha256Digest(release.Checksum);
    if (expectedDigest.isEmpty() || !VerifySha256(stageFilePath, expectedDigest))
    {
        nh_log("Checksum mismatch for %s", qPrintable(plugin.PluginId));
        QFile::remove(stageFilePath);
        return {};
    }

    if (!ExtractArchive(stageFilePath, mergeDirPath))
    {
        nh_log("Failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        return {};
    }

    nh_log("Staged %s for %s (checksum %s)", qPrintable(release.TagName), qPrintable(plugin.PluginId), qPrintable(release.Checksum));

    return release.TagName;
}

bool Utilities::FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath)
{
    if (!config.Save(NICKELUPDATER_CONF))
    {
        nh_log("Failed to save updated tags");
        return false;
    }

    const auto mergedArchivePath = MergedArchivePath();
    if (!CreateArchive(mergeDirPath, mergedArchivePath))
    {
        nh_log("Failed to create merged KoboRoot.tgz");
        return false;
    }

    if (!PublishArchive(mergedArchivePath))
    {
        nh_log("Failed to publish merged KoboRoot.tgz");
        return false;
    }

    if (!RebootDevice())
    {
        nh_log("Failed to reboot after publishing merged KoboRoot.tgz");
        return false;
    }

    nh_log("Published merged KoboRoot.tgz");
    return true;
}

void Utilities::CreateConfig(const char* filePath, const char* tmplFilePath)
{
    if (!QDir().mkpath(CONFIG_DIR))
    {
        nh_log("Failed to create config directory: %s", CONFIG_DIR);
        return;
    }

    if (QFile::exists(filePath))
    {
        return; // nothing to do
    }

    if (QFile::copy(tmplFilePath, filePath))
    {
        nh_log("Created config from template: %s", filePath);
    }
    else
    {
        nh_log("Failed to create config from template: %s -> %s", tmplFilePath, filePath);
    }
}

QString Utilities::MergeDirectoryPath()
{
    return QDir(CONFIG_DIR).filePath("staging/_merged_root");
}
