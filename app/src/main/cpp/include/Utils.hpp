#pragma once
#include "macros.hpp"
#include "dlfcn.h"
#include "cstddef"
#include "vector"
#include "filesystem"
#include "iostream"
#include "fstream"
#include "sstream"
#include "Il2Cpp.hpp"
#include "Il2Cpp-Structs.hpp"

namespace Utils {
    namespace fs = std::filesystem;

#include <vector>
#include <string>
    static const std::vector<std::string> keywords = {
            "", "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept",
            "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
            "class", "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue",
            "contract_assert", "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do", "double",
            "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
            "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
            "operator", "or", "or_eq", "private", "protected", "public", "reflexpr", "register", "reinterpret_cast",
            "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
            "switch", "synchronized", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid",
            "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",

            "abstract", "add", "as", "base", "byte", "checked", "decimal", "delegate", "event", "explicit", "extern",
            "finally", "fixed", "foreach", "implicit", "in", "interface", "internal", "is", "lock", "null", "object",
            "out", "override", "params", "readonly", "ref", "remove", "sbyte", "sealed", "stackalloc", "string",
            "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort", "using static", "value", "when", "where",
            "yield",

            "INT32_MAX", "INT32_MIN", "UINT32_MAX", "UINT16_MAX", "INT16_MAX", "UINT8_MAX", "INT8_MAX", "INT_MAX",
            "Assert", "NULL", "O"
    };


    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    bool isNum(char c) {
        return (c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9' || c == '0');
    }

    std::string FixName(const char* name) {
        std::string newName;
        const char* validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_";
        std::string oldName(name);

        if (isNum(oldName[0])) {
            oldName = "$" + oldName;
        }

        for (char c : oldName) {
            if (std::strchr(validChars, c)) {
                newName += c;
            } else if (c == '`') {
                newName += "_";
            } else {
                newName += "$";
            }
        }
        if (newName.find("k__BackingField") != std::string::npos) {
            replaceAll(newName, "k__BackingField", "");
        }

        if (std::find(keywords.begin(), keywords.end(), newName) != keywords.end()) {
            newName = "$" + newName;
        }

        return newName;
    }

    std::string GetPackageName() {
        char application_id[256] = {0};
        FILE *fp = fopen("/proc/self/cmdline", "r");
        if (fp) {
            fread(application_id, 1, sizeof(application_id) - 1, fp);
            fclose(fp);
        }
        return std::string(application_id);
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (std::string::npos == first) {
            return str;
        }
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    std::string generateSafeParamName(const char* originalName, int index) {
        if (!originalName) return "param" + std::to_string(index);

        std::string fixed = Utils::FixName(originalName);
        std::string trimmed = Utils::trim(fixed);

        if (trimmed.empty()) {
            return "param" + std::to_string(index);
        }
        return trimmed;
    }

    std::string GetCppNameFromIl2CppType(void* klass, void* parentKlass = nullptr) {
        if (!klass) return "sus";

        auto classType = Il2Cpp::il2cpp_class_get_type(klass);

        std::string rawName = Il2Cpp::il2cpp_class_get_name(klass);
        bool isArray = rawName.ends_with("[]");

        std::string namespaze = Il2Cpp::il2cpp_class_get_namespace(klass);
        if (namespaze.empty()) {
            namespaze = "GlobalNamespace";
        }

        std::string name = rawName;
        if (isArray) {
            name = name.substr(0, name.length() - 2);
        }

        bool isVT = Il2Cpp::il2cpp_class_is_valuetype(parentKlass);
        bool isEnum = Il2Cpp::il2cpp_class_is_enum(parentKlass);
        bool dontPtr = (isVT || isEnum);

        name = FixName(name.c_str());
        std::string fullName = namespaze + "." + name;
        std::string ret;

        if (fullName == "System.Void") {
            ret = "void";
        } else if (fullName == "System.String") {
            if (nullptr != parentKlass) {
                if (dontPtr) {
                    ret = "::BNM::Structures::Mono::String*";
                }
            } else {
                ret = "std::string";
            }
        } else if (fullName == "System.Int32") {
            ret = "int";
        } else if (fullName == "System.Int8") {
            ret = "int8_t";
        } else if (fullName == "System.UInt8") {
            ret = "uint8_t";
        } else if (fullName == "System.Int16") {
            ret = "int16_t";
        } else if (fullName == "System.UInt16") {
            ret = "uint16_t";
        } else if (fullName == "System.UInt32") {
            ret = "uint32_t";
        } else if (fullName == "System.Int64") {
            ret = "int64_t";
        } else if (fullName == "System.UInt64") {
            ret = "uint64_t";
        } else if (fullName == "System.Single") {
            ret = "float";
        } else if (fullName == "System.Double") {
            ret = "double";
        } else if (fullName == "System.Boolean") {
            ret = "bool";
        } else if (fullName == "System.IntPtr") {
            ret = "uintptr_t";
        } else if (fullName == "System.Decimal") {
            ret = "::BNM::Types::decimal";
        } else if (fullName == "System.Char") {
            ret = "char";
        } else if (fullName == "System.Byte") {
            ret = "::BNM::Types::byte";
        } else if (fullName == "System.SByte") {
            ret = "::BNM::Types::sbyte";
        } else if (fullName == "System.Type") {
            ret = "::BNM::MonoType*";
        } else if (fullName == "System.Object") {
            ret = "::BNM::IL2CPP::Il2CppObject*";
        } else if (fullName == "UnityEngine.Vector2") {
            ret = "::BNM::Structures::Unity::Vector2";
        } else if (fullName == "UnityEngine.Vector3") {
            ret = "::BNM::Structures::Unity::Vector3";
        } else if (fullName == "UnityEngine.Quaternion") {
            ret = "::BNM::Structures::Unity::Quaternion";
        } else if (fullName == "UnityEngine.Rect") {
            ret = "::BNM::Structures::Unity::Rect";
        } else if (fullName == "UnityEngine.Color") {
            ret = "::BNM::Structures::Unity::Color";
        } else if (fullName == "UnityEngine.Color32") {
            ret = "::BNM::Structures::Unity::Color32";
        } else if (fullName == "System.Array") {
            ret = "::BNM::IL2CPP::Il2CppObject*";
        } else if (fullName == "System.Collections.Generic.List") {
            ret = "::BNM::IL2CPP::Il2CppObject*";
        } else if (fullName == "UnityEngine.Matrix4x4") {
            ret = "::BNM::Structures::Unity::Matrix4x4";
        } else if (fullName == "UnityEngine.Matrix3x3") {
            ret = "::BNM::Structures::Unity::Matrix3x3";
        } else {
            if (Il2Cpp::il2cpp_class_is_enum(klass)) {
                std::string baseType = "int";
                void* iter = nullptr;
                void* field = Il2Cpp::il2cpp_class_get_fields(klass, &iter);
                if (field) {
                    auto fieldClass = Il2Cpp::il2cpp_class_from_type(Il2Cpp::il2cpp_field_get_type(field));
                    baseType = GetCppNameFromIl2CppType(fieldClass);
                }
                ret = baseType;
            } else if (Il2Cpp::il2cpp_type_is_byref(classType)) {
                auto ogType = Il2Cpp::il2cpp_type_get_class_or_element_class(classType);
                if (ogType) {
                    ret = GetCppNameFromIl2CppType(ogType);
                    ret += "&";
                }
            }

            if (!Il2Cpp::il2cpp_class_is_valuetype(klass) &&
                !fullName.starts_with("UnityEngine::") &&
                !fullName.starts_with("BNM::")) {
                ret = "::BNM::IL2CPP::Il2CppObject*";
            }
        }

        if (ret == "") { // fallback here because idk :(
            ret = "::BNM::IL2CPP::Il2CppObject*";
        }

        if (isArray) {
            ret = "::BNM::Structures::Mono::Array<" + ret + ">*";
        } else {
            if (!dontPtr && !ret.ends_with("*") && ret != "void") {
                ret += "*";
            }
        }

        if (parentKlass != nullptr &&
            std::format("{}.{}", Il2Cpp::il2cpp_class_get_namespace(parentKlass), Il2Cpp::il2cpp_class_get_name(parentKlass)) ==
            std::format("{}.{}", Il2Cpp::il2cpp_class_get_namespace(klass), Il2Cpp::il2cpp_class_get_name(klass))) {
            std::string nspace = "";
            const char* dfault = Il2Cpp::il2cpp_class_get_namespace(klass);
            if (strcmp(dfault, "") == 0) {
                nspace = "GlobalNamespace";
            } else {
                nspace = std::string(dfault);
            }
            std::string n = Utils::FixName(Il2Cpp::il2cpp_class_get_name(klass));
            ret = std::format("{}", n);
        }

        if (ret == "std::string*") return "std::string";

        return ret;
    }

    std::string GetDir(const std::vector<std::string>& args) {
        std::string SDKDir = "/storage/emulated/0/Android/data/" + std::string(GetPackageName()) + "/SDK";

        if (!args.empty()) {
            SDKDir += "/";
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) SDKDir += "/";
                SDKDir += args[i];
            }
        }

        bool isFile = (SDKDir.size() >= 2 && SDKDir.substr(SDKDir.size() - 2) == ".h") ||
                      (SDKDir.size() >= 4 && SDKDir.substr(SDKDir.size() - 4) == ".hpp") ||
                      (SDKDir.size() >= 4 && SDKDir.substr(SDKDir.size() - 4) == ".cpp");

        if (isFile && !fs::exists(fs::path(SDKDir).parent_path())) {
            fs::create_directories(fs::path(SDKDir).parent_path());
        }

        if (!isFile && !fs::exists(SDKDir)) {
            fs::create_directories(SDKDir);
        }

        return SDKDir;
    }
}