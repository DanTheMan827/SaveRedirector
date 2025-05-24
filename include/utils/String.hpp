#pragma once

#include <audica-hook/audica.h>

#include <audica-hook/utils/strings.hpp>
#include <string>
#include <type_traits>

namespace DanTheMan827::SaveRedirector::Utils {
    class StringWrapper {
       private:
        std::string stdStr;

       public:
        StringWrapper(std::string str) {
            stdStr = str;
        }

        StringWrapper(Il2CppString* il2cppStr) {
            stdStr = ::AudicaHook::Utils::to_utf8(::AudicaHook::Utils::csstrtostr(il2cppStr));
        }

        StringWrapper(System_String_o* sysStr) {
            stdStr = ::AudicaHook::Utils::to_utf8(::AudicaHook::Utils::csstrtostr(sysStr));
        }

        // Implicit conversion to std::string
        operator std::string() {
            return stdStr;
        }

        // Implicit conversion to Il2CppString*
        operator Il2CppString*() {
            return (Il2CppString*) il2cpp_utils::createcsstr(stdStr);
        }

        // Implicit conversion to System_String_o*
        operator System_String_o*() {
            return (System_String_o*) il2cpp_utils::createcsstr(stdStr);
        }

        // Assignment from std::string
        StringWrapper& operator=(std::string& str) {
            stdStr.assign(str);
            return *this;
        }

        // Assignment from Il2CppString*
        StringWrapper& operator=(Il2CppString* il2cppStr) {
            stdStr.assign(::AudicaHook::Utils::to_utf8(::AudicaHook::Utils::csstrtostr(il2cppStr)));
            return *this;
        }

        // Assignment from System_String_o*
        StringWrapper& operator=(System_String_o* sysStr) {
            stdStr.assign(::AudicaHook::Utils::to_utf8(::AudicaHook::Utils::csstrtostr(sysStr)));
            return *this;
        }

        operator char const*() {
            return stdStr.c_str();
        }

        // Accessor
        std::string str() {
            return stdStr;
        }
    };
}  // namespace DanTheMan827::SaveRedirector::Utils
