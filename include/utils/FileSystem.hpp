#pragma once

#include <dlfcn.h>
#include <link.h>
#include <scotland2/shared/modloader.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <audica-hook/utils/filesystem.hpp>
#include <cstring>
#include <scotland2/shared/loader.hpp>
#include <string>

#include "modInfo.hpp"

namespace DanTheMan827::SaveRedirector::Utils::FileSystem {
    inline std::string getDataDir() {
        static auto dataDir = AudicaHook::Utils::FileSystem::getDataDir(modInfo.id.c_str());
        return dataDir;
    }

    inline std::string getPlayerPrefsConfigPath() {
        static std::string playerPrefsConfigPath = fmt::format("{}/{}", getDataDir(), "player_prefs.json");
        return playerPrefsConfigPath;
    }
}  // namespace DanTheMan827::SaveRedirector::Utils::FileSystem
