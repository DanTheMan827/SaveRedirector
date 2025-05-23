#pragma once

#include <audica-hook/audica.h>

#include <audica-hook/utils/strings.hpp>
#include <string>
#include <type_traits>

namespace DanTheMan827::SaveRedirector::Utils::String {
    template <typename T>
    requires(std::is_same_v<T, Il2CppString*> || std::is_same_v<T, System_String_o*>)
    inline std::string convert_il2cpp(T il2cpp_str) {
        return ::AudicaHook::Utils::to_utf8(::AudicaHook::Utils::csstrtostr((T) (il2cpp_str)));
    }

    template <typename T = Il2CppString*>
    requires(std::is_same_v<T, Il2CppString*> || std::is_same_v<T, System_String_o*>)
    inline T convert_il2cpp(std::string const& std_str) {
        return (T) il2cpp_utils::createcsstr((std_str));
    }
}  // namespace DanTheMan827::SaveRedirector::Utils::String
