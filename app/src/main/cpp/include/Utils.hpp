#pragma once
#include "macros.hpp"
#include "dlfcn.h"
#include "cstddef"
#include "vector"
#include "filesystem"
#include <fmt/format.h>
#include "iostream"
#include "fstream"
#include "sstream"
#include "Il2Cpp.hpp"
#include "Il2Cpp-Structs.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>


namespace Utils {
    namespace fs = std::filesystem;

    static const std::unordered_map<std::string, std::string> DefaultTypeMap {
            { "System.Boolean", "bool" },
            { "System.Int32", "int" },
            { "System.Single", "float" },
            { "System.IntPtr", "::BNM::Types::nint" },
            { "System.UInt32", "::BNM::Types::uint" },
            { "System.Byte", "::BNM::Types::byte" },
            { "System.Void", "void" },
            { "System.UInt64", "uint64_t" },
            { "System.Int64", "int64_t" },
            { "System.Int16", "int16_t" },
            { "System.Char", "uint8_t" },
            { "System.SByte", "::BNM::Types::sbyte" },
            { "System.UInt16", "::BNM::Types::ushort" },
            { "System.UIntPtr", "::BNM::Types::nuint" },
            { "System.Double", "::BNM::Types::decimal" },
            //{ "System.String", "BNM::Structures::Mono::String*" }, have a string setup for this. should add a toggle or deprecate entirely due to instability and inconsistency.
            { "System.Object", "::BNM::IL2CPP::Il2CppObject*" },
            { "UnityEngine.Vector4", "::BNM::Structures::Unity::Vector4" },
            { "UnityEngine.Vector3", "::BNM::Structures::Unity::Vector3" },
            { "UnityEngine.Vector2", "::BNM::Structures::Unity::Vector2" },
            { "UnityEngine.Quaternion", "::BNM::Structures::Unity::Quaternion" },
            { "UnityEngine.Rect", "::BNM::Structures::Unity::Rect" },
            { "UnityEngine.Matrix4x4", "::BNM::Structures::Unity::Matrix4x4" },
            { "UnityEngine.Matrix3x3", "::BNM::Structures::Unity::Matrix3x3" },
            { "UnityEngine.Color", "::BNM::Structures::Unity::Color" },
            { "UnityEngine.RaycastHit", "::BNM::Structures::Unity::RaycastHit" },
            { "UnityEngine.Ray", "::BNM::Structures::Unity::Ray" },
            { "System.Type", "::BNM::MonoType*" }
    };

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
            "Assert", "NULL", "O", "INT_MIN", "INT_MAX"
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

    std::string GetClassGetter(void* klass, const std::string& icn = "") {
        std::vector<void*> defTree;
        void* current = klass;
        while (current != nullptr) {
            defTree.push_back(current);
            current = Il2Cpp::il2cpp_class_get_declaring_type(current);
        }
        std::reverse(defTree.begin(), defTree.end());

        std::string builder;
        void* firstType = defTree[0];

        const char* ns = Il2Cpp::il2cpp_class_get_namespace(firstType);
        const char* n = Il2Cpp::il2cpp_class_get_name(firstType);

        if (icn.empty()) {
            builder += fmt::format("::BNM::Class(\"{}\", \"{}\")", ns ? ns : "", n ? n : "");
        } else {
            builder += fmt::format("::BNM::Class {} = ::BNM::Class(\"{}\", \"{}\")", icn, ns ? ns : "", n ? n : "");
        }

        if (defTree.size() > 1) {
            for (int i = 1; i < (int)defTree.size(); ++i) {
                const char* innerName = Il2Cpp::il2cpp_class_get_name(defTree[i]);
                builder += fmt::format(".GetInnerClass(\"{}\")", innerName ? innerName : "");
            }
        }

        return builder;
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

    std::string GetCppNameFromIl2CppType(void* klass, void* parentKlass = nullptr, CppCodeWriter* writer = nullptr) {
        if (!klass) return "invalid_type_name_error";

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

        bool isVT = Il2Cpp::il2cpp_class_is_valuetype(klass);
        bool isEnum = Il2Cpp::il2cpp_class_is_enum(klass);
        bool dontPtr = (isVT || isEnum);

        name = FixName(name.c_str());
        std::string fullName = namespaze + "." + name;
        std::string ret;

        if (DefaultTypeMap.contains(fullName)) {
            return DefaultTypeMap.at(fullName);
        } else if (fullName == "System.String") {
            if (parentKlass) {
                if (Il2Cpp::il2cpp_class_is_valuetype(parentKlass) || Il2Cpp::il2cpp_class_is_enum(parentKlass)) {
                    ret = "::BNM::Structures::Mono::String*";
                } else {
                    ret = "std::string";
                }
            } else {
                ret = "std::string";
            }
        } else {
            if (isEnum) {
                std::string baseType = "int";
                void* iter = nullptr;
                void* field = Il2Cpp::il2cpp_class_get_fields(klass, &iter);
                void* fieldClass = nullptr;
                if (field) {
                    fieldClass = Il2Cpp::il2cpp_class_from_type(Il2Cpp::il2cpp_field_get_type(field));
                    baseType = GetCppNameFromIl2CppType(fieldClass, nullptr, writer);
                }

                if (writer) {
                    std::string fileNamespace = namespaze;
                    if (fileNamespace.starts_with("UnityEngine")) {
                        fileNamespace = "C" + fileNamespace;
                    }
                    std::string includePath = fmt::format("#include<SDK/Include/{}/{}.hpp>", fileNamespace, name);
                    if (writer->preview().find(includePath) == std::string::npos) {
                        writer->WriteTopLine(includePath);
                    }
                    std::string ns = fileNamespace;
                    replaceAll(ns, ".", "::");
                    ret = fmt::format("::{}::{}", ns, name);
                } else {
                    ret = GetCppNameFromIl2CppType(fieldClass);
                }
            }
            else if (isVT) {
                if (parentKlass && !Il2Cpp::il2cpp_class_is_valuetype(parentKlass)) {
                    if (writer) {
                        std::string fileNamespace = namespaze;
                        if (fileNamespace.starts_with("UnityEngine")) {
                            fileNamespace = "C" + fileNamespace;
                        }
                        std::string includePath = fmt::format("#include<SDK/Include/{}/{}.hpp>", fileNamespace, name);
                        if (writer->preview().find(includePath) == std::string::npos) {
                            writer->WriteTopLine(includePath);
                        }
                        std::string ns = fileNamespace;
                        replaceAll(ns, ".", "::");
                        ret = fmt::format("::{}::{}", ns, name);
                    } else {
                        ret = "::BNM::IL2CPP::Il2CppObject*";
                    }
                } else {
                    ret = "::BNM::IL2CPP::Il2CppObject*";
                }
            }
            else if (Il2Cpp::il2cpp_type_is_byref(classType)) {
                auto ogType = Il2Cpp::il2cpp_type_get_class_or_element_class(classType);
                if (ogType) {
                    ret = GetCppNameFromIl2CppType(ogType, nullptr, writer);
                    ret += "&";
                }
            }

            if (!isVT && !fullName.starts_with("CUnityEngine::") && !fullName.starts_with("BNM::")) {
                ret = "::BNM::IL2CPP::Il2CppObject*";
            }
        }

        if (ret == "") { // fallback here because idk :(
            ret = "::BNM::IL2CPP::Il2CppObject*";
        }

        if (isArray) {
            ret = "::BNM::IL2CPP::Il2CppObject*";
        } else if (name.starts_with("System.Collections.Generic.List`1")) {
            ret = "::BNM::IL2CPP::Il2CppObject*";
        } else {
            if (!dontPtr && !ret.ends_with("*") && ret != "void") {
                ret += "*";
            }
        }

        if (parentKlass != nullptr &&
            fmt::format("{}.{}", Il2Cpp::il2cpp_class_get_namespace(parentKlass), Il2Cpp::il2cpp_class_get_name(parentKlass)) ==
            fmt::format("{}.{}", Il2Cpp::il2cpp_class_get_namespace(klass), Il2Cpp::il2cpp_class_get_name(klass))) {
            std::string nspace = "";
            const char* dfault = Il2Cpp::il2cpp_class_get_namespace(klass);
            if (strcmp(dfault, "") == 0) {
                nspace = "GlobalNamespace";
            } else if (std::string(dfault).starts_with("UnityEngine")) {
                nspace = "C" + std::string(dfault);
            } else if (std::string(dfault).starts_with("::UnityEngine")) {
                auto dfstr = std::string(dfault);
                nspace = "::C" + dfstr.substr(2, dfstr.size());
            } else {
                nspace = std::string(dfault);
            }
            std::string n = Utils::FixName(Il2Cpp::il2cpp_class_get_name(klass));
            ret = fmt::format("{}", n);
            if (!dontPtr) {
                ret = ret + "*";
            }
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