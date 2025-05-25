#include "Hooks/SaveRedirection.hpp"

#include <audica-hook/utils/hooking.hpp>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <shared_mutex>

#include "logger.hpp"
#include "utils/FileSystem.hpp"
#include "utils/String.hpp"

namespace DanTheMan827::SaveRedirector::SaveRedirection {
    using namespace std;
    using StringWrapper = Utils::StringWrapper;
    using json = nlohmann::json;

    shared_mutex prefs_lock;
    json prefs_doc({});
    std::string prefs_path;

    void initPlayerPrefs() {
        static bool initialized = false;

        if (!initialized) {
            prefs_path = Utils::FileSystem::getPlayerPrefsConfigPath();

            if (AudicaHook::Utils::FileSystem::fileExists(prefs_path)) {
                ifstream file;

                file.open(prefs_path);

                if (file.is_open()) {
                    prefs_doc = json::parse(file);
                } else {
                    Logger.error("Failed to open PlayerPrefs file: {}", prefs_path);
                }
            } else {
                Logger.info("PlayerPrefs file not found: {}", prefs_path);
                initialized = true;
            }

            auto play_history_path = fmt::format("{}/{}", Utils::FileSystem::getDataDir(), "play_history.json");

            if (AudicaHook::Utils::FileSystem::fileExists(play_history_path)) {
                ifstream file;

                file.open(play_history_path);

                if (file.is_open()) {
                    auto play_history_doc = json::parse(file);

                    if (play_history_doc.contains("songs") && play_history_doc["songs"].is_array()) {
                        auto play_history = play_history_doc["songs"];

                        for (auto& song : play_history) {
                            std::string song_id = song["songID"];
                            auto song_history = song["history"];

                            for (auto& song_play : song_history) {
                                int32_t play_type = song_play["playType"];
                                bool no_fail = song_play["noFail"];
                                auto score_data = song_play["scoreData"];
                                int32_t score = score_data["score"];
                                int32_t difficulty = score_data["difficulty"];
                                float percent = score_data["percent"];
                                bool full_combo = score_data["fullCombo"];

                                if (score > 0 &&
                                    (!prefs_doc.contains(fmt::format("hs_{}", song_id)) || (prefs_doc[fmt::format("hs_{}", song_id)] < score))) {
                                    prefs_doc[fmt::format("hs_{}", song_id)] = score;
                                    prefs_doc[fmt::format("hs_diff_{}", song_id)] = difficulty;
                                    prefs_doc[fmt::format("hs_fullcombo_{}", song_id)] = full_combo ? 1 : 0;
                                    prefs_doc[fmt::format("hs_percent_{}", song_id)] = percent;
                                }
                            }
                        }
                    }
                } else {
                    Logger.error("Failed to open Play History file: {}", play_history_path);
                }
            }
        }
    }

    void savePlayerPrefs() {
        unique_lock<shared_mutex> lock(prefs_lock);
        static auto playerPrefsPath = Utils::FileSystem::getPlayerPrefsConfigPath();

        Logger.info("Saving PlayerPrefs to: {}", playerPrefsPath);

        // Write the JSON document to a file
        ofstream file;
        file.open(playerPrefsPath);

        if (!file.is_open()) {
            Logger.error("Failed to open PlayerPrefs file for writing: {}", playerPrefsPath);
            return;
        }

        file << prefs_doc.dump() << std::endl;

        file.close();
        Logger.info("PlayerPrefs saved");
    }

    template <typename T>
    concept PlayerPrefsType = is_same_v<T, int32_t> || is_same_v<T, float> || is_same_v<T, System_String_o*>;

    template <PlayerPrefsType T>
    T getPrefsValue(StringWrapper key, T defaultValue = T()) {
        shared_lock<shared_mutex> lock(prefs_lock);

        Logger.info("Read {}", key.str());

        if (!prefs_doc.contains(key)) {
            if constexpr (is_same_v<T, System_String_o*>) {
                auto wrapped_default = StringWrapper(defaultValue);
                Logger.info("{} not found, using default value: {}", key.str(), defaultValue ? wrapped_default.str() : "");
            } else {
                Logger.info("{} not found, using default value: {}", key.str(), defaultValue);
            }

            return defaultValue;
        }

        auto member = prefs_doc[static_cast<char const*>(key)];

        // Type-specific handling using template specialization
        if constexpr (is_same_v<T, int32_t>) {
            if (!member.is_number_integer()) {
                Logger.warn("{} is not int", key.str());
                return defaultValue;
            }

            auto value = member.get<int32_t>();
            Logger.info("int found: {} = {}", key.str(), value);
            return value;
        } else if constexpr (is_same_v<T, float>) {
            if (!(member.is_number())) {
                Logger.warn("{} is not float", key.str());
                return defaultValue;
            }

            auto value = member.get<float>();
            Logger.info("float found: {} = {}", key.str(), value);
            return value;
        } else if constexpr (is_same_v<T, System_String_o*>) {
            if (!member.is_string()) {
                Logger.warn("{} is not string", key.str());
                return defaultValue;
            }

            auto value = member.get<string>();
            Logger.info("PlayerPrefs string found: {} = {}", key.str(), value);
            return (T) (il2cpp_utils::createcsstr(value));
        }
    }

    template <PlayerPrefsType T>
    void setPrefsValue(StringWrapper key, T value) {
        {
            unique_lock<shared_mutex> lock(prefs_lock);

            if constexpr (is_same_v<T, int32_t>) {
                Logger.info("Set PlayerPrefs int: {} = {}", key.str(), value);
                prefs_doc[static_cast<char const*>(key)] = value;
            } else if constexpr (is_same_v<T, float>) {
                Logger.info("Set PlayerPrefs float: {} = {}", key.str(), value);
                prefs_doc[static_cast<char const*>(key)] = value;
            } else if constexpr (is_same_v<T, System_String_o*>) {
                auto wrappedValue = StringWrapper(value);
                Logger.info("Set PlayerPrefs string: {} = {}", key.str(), wrappedValue.str());
                prefs_doc[static_cast<char const*>(key)] = wrappedValue.str();
            }
        }

        savePlayerPrefs();
    }

    namespace Hooks {
        MAKE_EARLY_HOOK_FIND(PlayerPrefs_DeleteKey, "UnityEngine", "PlayerPrefs", "DeleteKey", void, System_String_o* key) {
            {
                unique_lock<shared_mutex> lock(prefs_lock);

                auto wrappedKey = StringWrapper(key);

                Logger.info("Delete {}", wrappedKey.str());

                if (prefs_doc.contains(wrappedKey)) {
                    prefs_doc.erase(wrappedKey);
                    Logger.info("Deleted {}", wrappedKey.str());
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
            setPrefsValue(key, value);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetInt, "UnityEngine", "PlayerPrefs", "SetInt", void, System_String_o* key, int32_t value) {
            setPrefsValue(key, value);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_SetString, "UnityEngine", "PlayerPrefs", "SetString", void, System_String_o* key, System_String_o* value) {
            setPrefsValue(key, value);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_HasKey, "UnityEngine", "PlayerPrefs", "HasKey", bool, System_String_o* key) {
            shared_lock<shared_mutex> lock(prefs_lock);

            auto wrapped_key = StringWrapper(key);
            Logger.info("Check PlayerPrefs has key: {}", wrapped_key.str());
            return prefs_doc.contains(wrapped_key);
        }

        MAKE_EARLY_HOOK_FIND(PlayerPrefs_Save, "UnityEngine", "PlayerPrefs", "Save", void) {
            savePlayerPrefs();
        }

        MAKE_EARLY_HOOK_FIND(SaveIO_GenName, "", "SaveIO", "GenName", System_String_o*, System_String_o* fileName, bool perSystem) {
            auto wrapped_filename = StringWrapper(fileName);
            Logger.info("SaveIO_GenName: {}, {} = {}", wrapped_filename.str(), perSystem, Utils::FileSystem::getDataDir());

            return StringWrapper(fmt::format("{}/{}", Utils::FileSystem::getDataDir(), wrapped_filename.str()));
        }
    }  // namespace Hooks
}  // namespace DanTheMan827::SaveRedirector::SaveRedirection
