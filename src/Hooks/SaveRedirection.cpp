#include "Hooks/SaveRedirection.hpp"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <audica-hook/utils/hooking.hpp>
#include <cstddef>
#include <shared_mutex>

#include "logger.hpp"
#include "utils/FileSystem.hpp"
#include "utils/String.hpp"

namespace DanTheMan827::SaveRedirector::SaveRedirection {
    using namespace std;
    using namespace Utils::String;

    shared_mutex prefs_lock;
    rapidjson::Document prefs_doc;
    std::string prefs_path = NULL;

    void initPlayerPrefs() {
        static bool initialized = false;

        if (!initialized) {
            prefs_path = Utils::FileSystem::getPlayerPrefsConfigPath();

            if (AudicaHook::Utils::FileSystem::fileExists(prefs_path)) {
                ifstream file;

                file.open(prefs_path);

                if (file.is_open()) {
                    rapidjson::IStreamWrapper is{file};
                    prefs_doc.ParseStream(is);
                    file.close();
                } else {
                    Logger.error("Failed to open PlayerPrefs file: {}", prefs_path.c_str());
                }
            } else {
                Logger.info("PlayerPrefs file not found: {}", prefs_path.c_str());

                // Load a blank document
                prefs_doc.SetObject();
                initialized = true;
            }
        }
    }

    void savePlayerPrefs() {
        unique_lock<shared_mutex> lock(prefs_lock);
        static auto playerPrefsPath = Utils::FileSystem::getPlayerPrefsConfigPath();

        Logger.info("Saving PlayerPrefs to: {}", playerPrefsPath.c_str());

        // Write the JSON document to a file
        ofstream file;
        file.open(playerPrefsPath);

        if (!file.is_open()) {
            Logger.error("Failed to open PlayerPrefs file for writing: {}", playerPrefsPath.c_str());
            return;
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        prefs_doc.Accept(writer);
        file << buffer.GetString();
        file.close();
        Logger.info("PlayerPrefs saved");
    }

    template <typename T>
    concept PlayerPrefsType = is_same_v<T, int32_t> || is_same_v<T, float> || is_same_v<T, System_String_o*>;

    template <PlayerPrefsType T>
    T getPrefsValue(System_String_o* key, T defaultValue = T()) {
        shared_lock<shared_mutex> lock(prefs_lock);

        auto keyStr = convert_il2cpp(key);
        auto keyConst = keyStr.c_str();

        Logger.info("Read {}", keyConst);

        if (!prefs_doc.HasMember(keyConst)) {
            if constexpr (is_same_v<T, System_String_o*>) {
                Logger.info("{} not found, using default value: {}", keyConst, defaultValue ? convert_il2cpp(defaultValue).c_str() : "");
            } else {
                Logger.info("{} not found, using default value: {}", keyConst, defaultValue);
            }

            return defaultValue;
        }

        auto const& member = prefs_doc[keyConst];

        // Type-specific handling using template specialization
        if constexpr (is_same_v<T, int32_t>) {
            if (!member.IsInt()) {
                Logger.warn("{} is not int", keyConst);
                return defaultValue;
            }

            auto value = member.GetInt();
            Logger.info("int found: {} = {}", keyConst, value);
            return value;
        } else if constexpr (is_same_v<T, float>) {
            if (!(member.IsFloat() || member.IsNumber())) {
                Logger.warn("{} is not float", keyConst);
                return defaultValue;
            }

            auto value = member.GetFloat();
            Logger.info("float found: {} = {}", keyConst, value);
            return value;
        } else if constexpr (is_same_v<T, System_String_o*>) {
            if (!member.IsString()) {
                Logger.warn("{} is not string", keyConst);
                return defaultValue;
            }

            auto value = member.GetString();
            Logger.info("PlayerPrefs string found: {} = {}", keyConst, value);
            return (T) (il2cpp_utils::createcsstr(value));
        }
    }

    template <PlayerPrefsType T>
    void setPrefsValue(string const& key, T value) {
        {
            unique_lock<shared_mutex> lock(prefs_lock);

            Logger.info("Set {}", key.c_str());

            if (!prefs_doc.HasMember(key.c_str())) {
                Logger.info("Creating {}", key.c_str());
                rapidjson::Value keyName(key.c_str(), prefs_doc.GetAllocator());
                rapidjson::Value newValue;

                if constexpr (is_same_v<T, int32_t> || is_same_v<T, float>) {
                    newValue = rapidjson::Value(rapidjson::kNumberType);
                } else if constexpr (is_same_v<T, System_String_o*>) {
                    newValue = rapidjson::Value(rapidjson::kStringType);
                }

                prefs_doc.AddMember(keyName, newValue, prefs_doc.GetAllocator());
            }

            if constexpr (is_same_v<T, int32_t>) {
                Logger.info("Set PlayerPrefs int: {} = {}", key.c_str(), value);
                prefs_doc[key.c_str()].SetInt(value);
            } else if constexpr (is_same_v<T, float>) {
                Logger.info("Set PlayerPrefs float: {} = {}", key.c_str(), value);
                prefs_doc[key.c_str()].SetFloat(value);
            } else if constexpr (is_same_v<T, System_String_o*>) {
                Logger.info("Set PlayerPrefs string: {} = {}", key.c_str(), convert_il2cpp(value).c_str());
                string valueStr = convert_il2cpp(value);
                prefs_doc[key.c_str()].SetString(valueStr.c_str(), prefs_doc.GetAllocator());
            }
        }

        savePlayerPrefs();
    }

    namespace Hooks {
        MAKE_EARLY_HOOK_FIND(PlayerPrefs_DeleteKey, "UnityEngine", "PlayerPrefs", "DeleteKey", void, System_String_o* key) {
            {
                unique_lock<shared_mutex> lock(prefs_lock);

                string keyStr = convert_il2cpp(key);
                auto keyConst = keyStr.c_str();

                Logger.info("Delete {}", keyConst);

                if (prefs_doc.HasMember(keyConst)) {
                    prefs_doc.RemoveMember(keyConst);
                    Logger.info("Deleted {}", keyConst);
                }
            }

            savePlayerPrefs();
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_GetFloat, "UnityEngine", "PlayerPrefs", "GetFloat", float, System_String_o* key, float defaultValue) {
            return getPrefsValue(key, defaultValue);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_GetInt, "UnityEngine", "PlayerPrefs", "GetInt", int32_t, System_String_o* key, int32_t defaultValue) {
            return getPrefsValue(key, defaultValue);
        }

        MAKE_EARLY_HOOK_FIND(
            PlayerPrefs_GetString, "UnityEngine", "PlayerPrefs", "GetString", System_String_o*, System_String_o* key, System_String_o* defaultValue
        ) {
            return getPrefsValue(key, defaultValue);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetFloat, "UnityEngine", "PlayerPrefs", "SetFloat", void, System_String_o* key, float value) {
            setPrefsValue(convert_il2cpp(key), value);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetInt, "UnityEngine", "PlayerPrefs", "SetInt", void, System_String_o* key, int32_t value) {
            setPrefsValue(convert_il2cpp(key), value);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetString, "UnityEngine", "PlayerPrefs", "SetString", void, System_String_o* key, System_String_o* value) {
            setPrefsValue(convert_il2cpp(key), value);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_HasKey, "UnityEngine", "PlayerPrefs", "HasKey", bool, System_String_o* key) {
            shared_lock<shared_mutex> lock(prefs_lock);

            string key_str = convert_il2cpp(key);
            Logger.info("Check PlayerPrefs has key: {}", key_str.c_str());
            return prefs_doc.HasMember(key_str.c_str());
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_Save, "UnityEngine", "PlayerPrefs", "Save", void) {
            savePlayerPrefs();
        }

        MAKE_EARLY_HOOK_FIND(SaveIO_GenName, "", "SaveIO", "GenName", System_String_o*, System_String_o* fileName, bool perSystem) {
            auto filename_str = convert_il2cpp(fileName);
            Logger.info("SaveIO_GenName: {}, {} = {}", filename_str.c_str(), perSystem, Utils::FileSystem::getDataDir());

            return convert_il2cpp<System_String_o*>(fmt::format("{}/{}", Utils::FileSystem::getDataDir(), filename_str));
        }
    }  // namespace Hooks
}  // namespace DanTheMan827::SaveRedirector::SaveRedirection
