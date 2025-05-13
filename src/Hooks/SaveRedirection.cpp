#include <audica-hook/utils/hooking.hpp>
#include <audica-hook/utils/strings.hpp>
#include <shared_mutex>

#include "logger.hpp"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "utils/FileSystem.hpp"

namespace DanTheMan827::SaveRedirection {
    using namespace std;
    using namespace AudicaHook::Utils::Strings;

    static shared_mutex prefsLock;
    static unordered_map<string, rapidjson::Value> playerPrefsData;

    rapidjson::Document* GetPlayerPrefs() {
        static bool initialized = false;
        static rapidjson::Document playerPrefsDoc;

        if (!initialized) {
            auto playerPrefsPath = Utils::FileSystem::playerPrefsConfigPath;

            if (AudicaHook::Utils::FileSystem::fileExists(playerPrefsPath)) {
                ifstream file;

                file.open(playerPrefsPath);

                if (file.is_open()) {
                    rapidjson::IStreamWrapper is{file};
                    playerPrefsDoc.ParseStream(is);
                    file.close();
                } else {
                    Logger.error("Failed to open PlayerPrefs file: {}", playerPrefsPath.c_str());
                }
            } else {
                Logger.info("PlayerPrefs file not found: {}", playerPrefsPath.c_str());

                // Load a blank document
                playerPrefsDoc.SetObject();
                initialized = true;
            }
        }

        return &playerPrefsDoc;
    }

    void SavePlayerPrefs() {
        thread saveThread([]() {
            unique_lock<shared_mutex> lock(prefsLock);
            auto playerPrefsDoc = GetPlayerPrefs();
            auto playerPrefsPath = Utils::FileSystem::playerPrefsConfigPath;

            Logger.info("Saving PlayerPrefs to: {}", playerPrefsPath.c_str());

            // Write the JSON document to a file
            ofstream file;
            file.open(playerPrefsPath);

            if (file.is_open()) {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                (*playerPrefsDoc).Accept(writer);
                file << buffer.GetString();
                file.close();
                Logger.info("PlayerPrefs saved");
            } else {
                Logger.error("Failed to open PlayerPrefs file for writing: {}", playerPrefsPath.c_str());
            }
        });

        saveThread.detach();
        Logger.info("PlayerPrefs save thread started");
    }

    template <typename T>
    T GetPlayerPrefsValue(System_String_o* key, T defaultValue = T()) {
        shared_lock<shared_mutex> lock(prefsLock);
        auto playerPrefsDoc = GetPlayerPrefs();
        string keyStr = il2cpp_to_std(key);

        Logger.info("Check PlayerPrefs key: {}", keyStr.c_str());

        if ((*playerPrefsDoc).HasMember(keyStr.c_str())) {
            auto const& member = (*playerPrefsDoc)[keyStr.c_str()];

            // Type-specific handling using template specialization
            if constexpr (is_same_v<T, int32_t>) {
                Logger.info("Get PlayerPrefs int: {}", keyStr.c_str());
                if (member.IsInt()) {
                    auto value = member.GetInt();
                    Logger.info("PlayerPrefs int found: {} = {}", keyStr.c_str(), value);
                    return value;
                }
            } else if constexpr (is_same_v<T, float>) {
                Logger.info("Get PlayerPrefs float: {}", keyStr.c_str());
                if (member.IsFloat() || member.IsNumber()) {
                    auto value = member.GetFloat();
                    Logger.info("PlayerPrefs float found: {} = {}", keyStr.c_str(), value);
                    return value;
                }
            } else if constexpr (is_same_v<T, System_String_o*>) {
                Logger.info("Get PlayerPrefs string: {}", keyStr.c_str());
                if (member.IsString()) {
                    auto value = member.GetString();
                    Logger.info("PlayerPrefs string found: {} = {}", keyStr.c_str(), value);
                    return (T) (il2cpp_utils::createcsstr(value));
                }
            }
        }

        // Log default value based on type
        if constexpr (is_same_v<T, System_String_o*>) {
            Logger.info("Using default value: {}", defaultValue ? il2cpp_to_std(defaultValue).c_str() : "");
        } else {
            Logger.info("Using default value: {}", defaultValue);
        }

        return defaultValue;
    }

    template <typename T>
    concept PlayerPrefsType = is_same_v<T, int32_t> || is_same_v<T, float> || is_same_v<T, System_String_o*>;

    template <PlayerPrefsType T>
    void SetPlayerPrefsValue(string const& key, T value) {
        {
            unique_lock<shared_mutex> lock(prefsLock);
            auto playerPrefsDoc = GetPlayerPrefs();

            Logger.info("Set PlayerPrefs key: {}", key.c_str());

            if (!(*playerPrefsDoc).HasMember(key.c_str())) {
                Logger.info("Creating new PlayerPrefs key: {}", key.c_str());
                rapidjson::Value keyName(key.c_str(), (*playerPrefsDoc).GetAllocator());
                rapidjson::Value newValue;

                if constexpr (is_same_v<T, int32_t>) {
                    Logger.info("Set PlayerPrefs int: {} = {}", key.c_str(), value);
                    (*playerPrefsDoc)[key.c_str()].SetInt(value);
                } else if constexpr (is_same_v<T, float>) {
                    Logger.info("Set PlayerPrefs float: {} = {}", key.c_str(), value);
                    (*playerPrefsDoc)[key.c_str()].SetFloat(value);
                } else if constexpr (is_same_v<T, System_String_o*>) {
                    newValue = rapidjson::Value(rapidjson::kStringType);
                }

                (*playerPrefsDoc).AddMember(keyName, newValue, (*playerPrefsDoc).GetAllocator());
            }

            if constexpr (is_same_v<T, int32_t>) {
                Logger.info("Set PlayerPrefs int: {} = {}", key.c_str(), value);
                (*playerPrefsDoc)[key.c_str()].SetInt(value);
            } else if constexpr (is_same_v<T, float>) {
                Logger.info("Set PlayerPrefs float: {} = {}", key.c_str(), value);
                (*playerPrefsDoc)[key.c_str()].SetFloat(value);
            } else if constexpr (is_same_v<T, System_String_o*>) {
                Logger.info("Set PlayerPrefs string: {} = {}", key.c_str(), il2cpp_to_std(value).c_str());
                string valueStr = il2cpp_to_std(value);
                (*playerPrefsDoc)[key.c_str()].SetString(valueStr.c_str(), (*playerPrefsDoc).GetAllocator());
            }
        }

        SavePlayerPrefs();
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_DeleteKey, "UnityEngine", "PlayerPrefs", "DeleteKey", void, System_String_o* key) {
        unique_lock<shared_mutex> lock(prefsLock);

        auto playerPrefsDoc = GetPlayerPrefs();
        string keyStr = il2cpp_to_std(key);
        Logger.info("Delete PlayerPrefs key: {}", keyStr.c_str());
        if ((*playerPrefsDoc).HasMember(keyStr.c_str())) {
            (*playerPrefsDoc).RemoveMember(keyStr.c_str());
            Logger.info("Deleted PlayerPrefs key: {}", keyStr.c_str());
        }
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_GetFloat, "UnityEngine", "PlayerPrefs", "GetFloat", float, System_String_o* key, float defaultValue) {
        return GetPlayerPrefsValue<float>(key, defaultValue);
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_GetInt, "UnityEngine", "PlayerPrefs", "GetInt", int32_t, System_String_o* key, int32_t defaultValue) {
        return GetPlayerPrefsValue<int32_t>(key, defaultValue);
    }

    MAKE_EARLY_HOOK_FIND(
        PlayerPrefs_GetString, "UnityEngine", "PlayerPrefs", "GetString", System_String_o*, System_String_o* key, System_String_o* defaultValue
    ) {
        return GetPlayerPrefsValue<System_String_o*>(key, defaultValue);
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetFloat, "UnityEngine", "PlayerPrefs", "SetFloat", void, System_String_o* key, float value) {
        SetPlayerPrefsValue(il2cpp_to_std(key), value);
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetInt, "UnityEngine", "PlayerPrefs", "SetInt", void, System_String_o* key, int32_t value) {
        SetPlayerPrefsValue(il2cpp_to_std(key), value);
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetString, "UnityEngine", "PlayerPrefs", "SetString", void, System_String_o* key, System_String_o* value) {
        SetPlayerPrefsValue(il2cpp_to_std(key), value);
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_HasKey, "UnityEngine", "PlayerPrefs", "HasKey", bool, System_String_o* key) {
        shared_lock<shared_mutex> lock(prefsLock);

        auto playerPrefsDoc = GetPlayerPrefs();
        string keyStr = il2cpp_to_std(key);
        Logger.info("Check PlayerPrefs has key: {}", keyStr.c_str());
        return (*playerPrefsDoc).HasMember(keyStr.c_str());
    }

    MAKE_EARLY_HOOK_FIND(PlayerPrefs_Save, "UnityEngine", "PlayerPrefs", "Save", void) {
        SavePlayerPrefs();
    }

    MAKE_EARLY_HOOK_FIND(SaveIO_GenName, "", "SaveIO", "GenName", System_String_o*, System_String_o* fileName, bool perSystem) {
        auto fileNameStr = il2cpp_to_std(fileName);
        Logger.info("SaveIO_GenName: {}, {} = {}", fileNameStr.c_str(), perSystem, Utils::FileSystem::dataDir);

        return std_to_il2cpp<System_String_o*>(fmt::format("{}/{}", Utils::FileSystem::dataDir, fileNameStr));
    }

}  // namespace DanTheMan827::SaveRedirection
