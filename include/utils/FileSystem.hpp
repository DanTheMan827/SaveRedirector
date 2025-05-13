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
#include <DanTheMan827/Macros/DEFINE_GET_PROPERTY.hpp>
#include <scotland2/shared/loader.hpp>
#include <string>

#include "modInfo.hpp"

namespace DanTheMan827::SaveRedirection::Utils {
    class FileSystem {
        DEFINE_GET_PROPERTY(dataDir, std::string, {
            static auto dataDir = AudicaHook::Utils::FileSystem::getDataDir(modInfo.id.c_str());
            return dataDir;
        })

        DEFINE_GET_PROPERTY(playerPrefsConfigPath, std::string, {
            static std::string playerPrefsConfigPath = fmt::format("{}/{}", dataDir, "player_prefs.json");
            return playerPrefsConfigPath;
        })
    };
}  // namespace DanTheMan827::SaveRedirection::Utils
