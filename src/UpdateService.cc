#include "UpdateService.h"
#include "Constants.h"
#include "GitHubInterface.h"
#include <NickelHook.h>
#include <QDir>
#include <QFile>
#include <QProcess>

UpdateService::Result UpdateService::Run(UserConfig& config, HttpClient& httpClient)
{
    const auto mergeDirPath = QDir(STAGING_DIR).filePath("_merged_root");
    if (!PrepareMergeDirectory(mergeDirPath))
    {
        return Result::Failed;
    }

    bool hasUpdates = false;
    bool hadFailures = false;
    for (const auto& plugin : config.GetPlugins())
    {
        const auto result = StagePluginUpdate(httpClient, plugin, mergeDirPath);
        if (result.Status == PluginUpdateStatus::Failed)
        {
            hadFailures = true;
            continue;
        }

        if (result.Status == PluginUpdateStatus::Unchanged)
        {
            continue;
        }

        config.SetTag(plugin.PluginId, result.TagName);
        hasUpdates = true;
    }

    if (!hasUpdates)
    {
        return hadFailures ? Result::Failed : Result::NoUpdates;
    }

    return PublishMergedUpdate(config, mergeDirPath) ? Result::Updated : Result::Failed;
}

bool UpdateService::PrepareMergeDirectory(const QString& mergeDirPath)
{
    QDir stagingRoot(STAGING_DIR);
    if (stagingRoot.exists() && !stagingRoot.removeRecursively())
    {
        nh_log("Failed to clear staging directory: %s", STAGING_DIR);
        return false;
    }

    if (!QDir().mkpath(mergeDirPath))
    {
        nh_log("Failed to create merge directory: %s", qPrintable(mergeDirPath));
        return false;
    }

    return true;
}

UpdateService::PluginUpdateResult UpdateService::StagePluginUpdate(HttpClient& httpClient, const PluginConfigEntry& plugin, const QString& mergeDirPath)
{
    const auto release = GitHubInterface::GetLatestRelease(httpClient, plugin.PluginId);
    if (!release.IsValid())
    {
        nh_log("Failed to load latest release for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}};
    }

    if (!plugin.TagName.isEmpty() && plugin.TagName == release.TagName)
    {
        nh_log("Plugin %s already at %s", qPrintable(plugin.PluginId), qPrintable(plugin.TagName));
        return {PluginUpdateStatus::Unchanged, {}};
    }

    const auto stageDirPath = QDir(STAGING_DIR).filePath(plugin.PluginId);
    if (!QDir().mkpath(stageDirPath))
    {
        nh_log("Failed to create stage dir for %s", qPrintable(plugin.PluginId));
        return {PluginUpdateStatus::Failed, {}};
    }

    const auto stageFilePath = QDir(stageDirPath).filePath("KoboRoot.tgz");
    if (!DownloadFile(httpClient, release.KoboRootUrl, stageFilePath))
    {
        nh_log("Failed to download KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        QDir(stageDirPath).removeRecursively();
        return {PluginUpdateStatus::Failed, {}};
    }

    if (!RunProcess("tar", {"-xzf", stageFilePath, "-C", mergeDirPath}))
    {
        nh_log("Failed to extract KoboRoot.tgz for %s", qPrintable(plugin.PluginId));
        QDir(stageDirPath).removeRecursively();
        return {PluginUpdateStatus::Failed, {}};
    }

    nh_log("Staged %s for %s", qPrintable(release.TagName), qPrintable(plugin.PluginId));
    return {PluginUpdateStatus::Updated, release.TagName};
}

bool UpdateService::DownloadFile(HttpClient& httpClient, const QString& url, const QString& outputPath)
{
    QByteArray output;
    if (!httpClient.Get(url, &output, "*/*"))
    {
        return false;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        nh_log("Failed to open %s for writing", qPrintable(outputPath));
        return false;
    }

    return file.write(output) == output.size();
}

bool UpdateService::PublishMergedUpdate(const UserConfig& config, const QString& mergeDirPath)
{
    const auto mergedArchivePath = QDir(STAGING_DIR).filePath("KoboRoot.merged.tgz");
    QFile::remove(mergedArchivePath);
    if (!RunProcess("tar", {"-czf", mergedArchivePath, "-C", mergeDirPath, "."}))
    {
        nh_log("Failed to create merged KoboRoot.tgz");
        return false;
    }

    QFile::remove(KOBOROOT_PATH);
    if (!QFile::copy(mergedArchivePath, KOBOROOT_PATH))
    {
        nh_log("Failed to publish merged KoboRoot.tgz");
        return false;
    }

    if (!config.Save(NICKELUPDATER_CONF))
    {
        nh_log("Failed to save updated tags");
        return false;
    }

    if (!QProcess::startDetached("reboot"))
    {
        nh_log("Failed to reboot after publishing merged KoboRoot.tgz");
        return false;
    }

    nh_log("Published merged KoboRoot.tgz");
    return true;
}

bool UpdateService::RunProcess(const QString& program, const QStringList& args, QByteArray* output)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        nh_log("%s %s failed (exit code %d): %s", qPrintable(program), qPrintable(args.join(' ')), process.exitCode(), process.readAllStandardError().constData());
        return false;
    }

    if (output != nullptr)
    {
        *output = process.readAllStandardOutput();
    }

    return true;
}
