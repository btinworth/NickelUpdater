#pragma once

#include "UserConfig.h"
#include <QString>

class Utilities
{
public:
    static QString ProcessPluginUpdate(const PluginConfigEntry& plugin, const QString& mergeDirPath);

    static bool FinalizeAndApplyUpdates(const UserConfig& config, const QString& mergeDirPath);
    static bool PrepareMergeDirectory(const QString& mergeDirPath);

    static void CreateConfig(const char* filePath, const char* tmplFilePath);
    static QString MergeDirectoryPath();
};
