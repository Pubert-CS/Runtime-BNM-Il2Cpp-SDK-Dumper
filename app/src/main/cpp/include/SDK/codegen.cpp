#include "SDK/codegen.hpp"
#include "macros.hpp"
#include "dlfcn.h"
#include "cstddef"
#include "CppCodeWriter.hpp"
#include "vector"
#include "filesystem"
#include "fstream"
#include "sstream"
#include <fmt/format.h>
#include "Utils.hpp"
#include "format"
#include "flags.h"
#include "unordered_map"
#include "set"
#include "KittyMemory/KittyInclude.hpp"

void* libHandle = nullptr;
ProcMap* mainMap = nullptr;

namespace Settings {
    size_t imageCount;
    int classCount = 0;
}

namespace fs = std::filesystem;

static std::set<std::string> createdNamespaceFiles;

std::vector<std::string> codegen::split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end;
    std::string token;
    std::vector<std::string> res;
    const size_t delim_len = delimiter.length();

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}
std::vector<std::string> codegen::getParamNames(void* method) {
    std::vector<std::string> names;
    uint32_t pcount = Il2Cpp::il2cpp_method_get_param_count(method);
    for (int i = 0; i < pcount; i++) {
        names.push_back(Utils::FixName(Il2Cpp::il2cpp_method_get_param_name(method, i)));
    }
    return names;
}
std::vector<std::string> codegen::getParamTypes(void* method) {
    std::vector<std::string> types;
    uint32_t pcount = Il2Cpp::il2cpp_method_get_param_count(method);
    for (int i = 0; i < pcount; i++) {
        const void* param = Il2Cpp::il2cpp_method_get_param(method, i);
        types.push_back(Utils::GetCppNameFromIl2CppType(Il2Cpp::il2cpp_class_from_type(param)));
    }
    return types;
}
std::string joinString(std::vector<std::string> args, std::string joiner) {
    std::string ns = "";
    for (int i = 0; i < args.size(); i++) {
        ns += args[i];
        if (i < args.size() - 1)
            ns += joiner;
    }
    return ns;
}

void codegen::start(void* handle, ProcMap map) {
    LOGI("Initializing gen");

    libHandle = handle;
    mainMap = &map;

    std::string path = Utils::GetDir({});

    if (fs::exists(path)) {
        fs::remove_all(path);
    }

    fs::create_directories(path);

    createdNamespaceFiles.clear();

    parseNoThreadCheck();

}

void codegen::parseNoThreadCheck() {
    auto domain = Il2Cpp::il2cpp_domain_get();
    size_t imgC = 0;
    auto assemblies = Il2Cpp::il2cpp_domain_get_assemblies(domain, &imgC);

    for (int i = 0; i < imgC; ++i) {
        auto image = Il2Cpp::il2cpp_assembly_get_image(assemblies[i]);
        LOGI("[+] Parsing %s", Il2Cpp::il2cpp_image_get_name(image));
        parseAssembly(image);
    }
    LOGI("[!] Finished running SDK Gen. Parsed %d classes.", Settings::classCount);
}

void codegen::parseAssembly(const void* image) {
    size_t count = Il2Cpp::il2cpp_image_get_class_count(image);
    for (int i = 0; i < count; ++i) {
        void* klass = const_cast<void*>(Il2Cpp::il2cpp_image_get_class(image, i));
        if (!klass) {
            continue;
        }
        parseClass(klass);
    }
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

void codegen::parseMethods(void* klass, CppCodeWriter& writer) {
    if (!klass) return;

    auto fullClassName = fmt::format("{}.{}", Il2Cpp::il2cpp_class_get_namespace(klass), Il2Cpp::il2cpp_class_get_name(klass));

    std::unordered_map<std::string, int> methodNameCounts;
    std::unordered_map<std::string, int> methodSignatureCounts;

    void* iter = nullptr;
    while (void* method = Il2Cpp::il2cpp_class_get_methods(klass, &iter)) {
        if (Il2Cpp::il2cpp_method_is_generic(method)) continue; // not handling allat.
        const char* name = Il2Cpp::il2cpp_method_get_name(method);
        const void* retType = Il2Cpp::il2cpp_method_get_return_type(method);
        const void* typeClass = Il2Cpp::il2cpp_class_from_type(retType);
        uint32_t methodIFlags = 0;
        uint32_t methodFlags = Il2Cpp::il2cpp_method_get_flags(method, &methodIFlags);

        bool needPtr = (Il2Cpp::il2cpp_class_is_valuetype(klass) || Il2Cpp::il2cpp_class_is_enum(klass));
        bool isStatic = methodFlags & METHOD_ATTRIBUTE_STATIC;

        std::string newName = "";
        if (strcmp(name, ".ctor") == 0) {
            newName = "new_ctor";
        } else if (strcmp(name, ".cctor") == 0) {
            newName = "new_cctor";
        } else {
            newName = std::string(name);
        }

        newName = Utils::FixName(newName.c_str());

        uint32_t paramCount = Il2Cpp::il2cpp_method_get_param_count(method);
        std::string signature = newName + "(";
        for (int i = 0; i < (int)paramCount; i++) {
            const void* paramType = Il2Cpp::il2cpp_method_get_param(method, i);
            const void* paramClass = Il2Cpp::il2cpp_class_from_type(paramType);
            
            std::string paramClassName = Utils::FixName(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(paramClass)));
            bool isParamDisplayClass = paramClassName.find("__c__DisplayClass") != std::string::npos ||
                                  paramClassName.find("__f__AnonymousType") != std::string::npos ||
                                  paramClassName.find("$$c__DisplayClass") != std::string::npos ||
                                  paramClassName.find("$$f__AnonymousType") != std::string::npos;
            
            std::string paramClassType;
            if (isParamDisplayClass) {
                paramClassType = "::BNM::IL2CPP::Il2CppObject*";
            } else {
                paramClassType = Utils::GetCppNameFromIl2CppType(const_cast<void*>(paramClass), klass, &writer);
            }

            if (Il2Cpp::il2cpp_type_is_byref(paramType)) {
                paramClassType += "&";
            }
            
            signature += paramClassType;

            if (i < (int)paramCount - 1) signature += ",";
        }
        signature += ")";

        if (methodSignatureCounts[signature]++ > 0) {
            newName += "_" + std::to_string(++methodNameCounts[newName]);
        }

        uint64_t staticVal = 0;
        std::ostringstream offsetStream;
        offsetStream << "0x" << std::hex << *(void**)method;

        std::ostringstream rvaStream;
        rvaStream << "0x" << std::hex << ((uintptr_t)*(void**)method - mainMap->startAddress);

        std::string typeClassName = Utils::FixName(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(typeClass)));
        bool isRetDisplayClass = typeClassName.find("__c__DisplayClass") != std::string::npos ||
                              typeClassName.find("__f__AnonymousType") != std::string::npos ||
                              typeClassName.find("$$c__DisplayClass") != std::string::npos ||
                              typeClassName.find("$$f__AnonymousType") != std::string::npos;

        writer.WriteComment("Original Type: " +
                            std::string(Il2Cpp::il2cpp_class_get_namespace(const_cast<void*>(typeClass))) + ":" +
                            std::string(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(typeClass))) +
                            " | Offset 0x" + offsetStream.str());

        if (newName.find("new_ctor") == 0 && !needPtr) {
            std::string cns = std::string(Il2Cpp::il2cpp_class_get_namespace(klass));
            std::string cn = Utils::FixName(Il2Cpp::il2cpp_class_get_name(klass));

            if (cns == "") {
                cns = "GlobalNamespace";
            } else if (cns.starts_with("UnityEngine")) {
                cns = "C" + cns;
            }

            std::string ctype = "::" + cns + "::" + cn;
            Utils::replaceAll(ctype, ".", "::");

            if (!needPtr) {
                ctype += "*";
            }

            writer.Write(fmt::format("static {} {}(", ctype, newName));
            std::vector<std::string> paramTypes;
            std::vector<std::string> paramNames;

            if (paramCount > 0) {
                for (int i = 0; i < (int)paramCount; ++i) {
                    const char* paramName = Il2Cpp::il2cpp_method_get_param_name(method, i);
                    const void* paramType = Il2Cpp::il2cpp_method_get_param(method, i);
                    const void* paramClass = Il2Cpp::il2cpp_class_from_type(paramType);

                    std::string safeParamName = Utils::generateSafeParamName(paramName, i);
                    
                    std::string paramClassName = Utils::FixName(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(paramClass)));
                    bool isParamDisplayClass = paramClassName.find("__c__DisplayClass") != std::string::npos ||
                                          paramClassName.find("__f__AnonymousType") != std::string::npos ||
                                          paramClassName.find("$$c__DisplayClass") != std::string::npos ||
                                          paramClassName.find("$$f__AnonymousType") != std::string::npos;
                    
                    std::string paramClassType;
                    if (isParamDisplayClass) {
                        paramClassType = "::BNM::IL2CPP::Il2CppObject*";
                    } else {
                        paramClassType = Utils::GetCppNameFromIl2CppType(const_cast<void*>(paramClass), klass, &writer);
                    }

                    int suffix = 0;
                    std::string originalName = safeParamName;
                    while (std::find(paramNames.begin(), paramNames.end(), safeParamName) != paramNames.end()) {
                        safeParamName = originalName + std::to_string(suffix++);
                    }

                    writer.Write(paramClassType + " " + safeParamName);
                    if (i < (int)paramCount - 1) {
                        writer.Write(", ");
                    }

                    paramTypes.push_back(paramClassType);
                    paramNames.push_back(safeParamName);
                }
            }
            writer.Write(")");
            writer.WriteBracket();
            writer.Write(fmt::format("return ({})StaticClass().CreateNewObjectParameters(", ctype));
            for (int i = 0; i < (int)paramNames.size(); i++) {
                if (paramTypes[i] == "std::string")
                    writer.Write(fmt::format("::BNM::CreateMonoString({})", paramNames[i]));
                else
                    writer.Write(paramNames[i]);
                if (i < (int)paramNames.size() - 1) {
                    writer.Write(", ");
                }
            }
            writer.WriteLine(");");
            writer.CloseBracket();
            continue;
        }

        std::string cppType;
        if (isRetDisplayClass) {
            cppType = "::BNM::IL2CPP::Il2CppObject*";
        } else {
            cppType = Utils::GetCppNameFromIl2CppType(const_cast<void*>(typeClass), klass, &writer);
        }

        std::string retClassName = Il2Cpp::il2cpp_class_get_name(const_cast<void*>(typeClass));
        bool retIsArray = std::string(retClassName).ends_with("[]");
        bool retIsList = std::string(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(typeClass)))
                         == "List`1";

        writer.WriteLine("template <typename T = " + cppType + ">");

        if (isStatic) {
            writer.Write("static ");
        }

        if (retIsArray) {
            writer.Write("::BNM::Structures::Mono::Array<T>* " + newName);
        } else if (retIsList) {
            writer.Write("::BNM::Structures::Mono::List<T>* " + newName);
        } else {
            writer.Write("T " + newName);
        }
        writer.Write("(");

        std::vector<std::string> paramTypes;
        std::vector<std::string> paramNames;
        std::vector<const void*> paramTypesReal;

        if (paramCount > 0) {
            for (int i = 0; i < (int)paramCount; ++i) {
                const char* paramName = Il2Cpp::il2cpp_method_get_param_name(method, i);
                const void* paramType = Il2Cpp::il2cpp_method_get_param(method, i);
                const void* paramClass = Il2Cpp::il2cpp_class_from_type(paramType);
                paramTypesReal.emplace_back(paramType);

                std::string safeParamName = Utils::generateSafeParamName(paramName, i);
                
                std::string paramClassName = Utils::FixName(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(paramClass)));
                bool isParamDisplayClass = paramClassName.find("__c__DisplayClass") != std::string::npos ||
                                           paramClassName.find("__f__AnonymousType") != std::string::npos ||
                                           paramClassName.find("$$c__DisplayClass") != std::string::npos ||
                                           paramClassName.find("$$f__AnonymousType") != std::string::npos;
                
                std::string paramClassType;
                if (isParamDisplayClass) {
                    paramClassType = "::BNM::IL2CPP::Il2CppObject*";
                } else {
                    paramClassType = Utils::GetCppNameFromIl2CppType(const_cast<void*>(paramClass), klass, &writer);
                }

                if (Il2Cpp::il2cpp_type_is_byref(paramType)) {
                    paramClassType += "&";
                }

                int suffix = 0;
                std::string originalName = safeParamName;
                while (std::find(paramNames.begin(), paramNames.end(), safeParamName) != paramNames.end()) {
                    safeParamName = originalName + std::to_string(suffix++);
                }

                writer.Write(paramClassType + " " + safeParamName);
                if (i < (int)paramCount - 1) {
                    writer.Write(", ");
                }

                paramTypes.push_back(paramClassType);
                paramNames.push_back(safeParamName);
            }
        }

        writer.Write(") ");
        writer.WriteBracket();

        std::string bnmMethodName = "__bnm_methodCall__0";

        if (methodFlags & METHOD_ATTRIBUTE_PINVOKE_IMPL) {
            if (!isStatic) {
                paramTypes.insert(paramTypes.begin(), "::BNM::IL2CPP::Il2CppObject*");
                paramNames.insert(paramNames.begin(), "this");
            }

            std::string ptypeStuff = "";
            std::string pnameStuff = "";
            for (int i = 0; i < (int)paramTypes.size(); i++) {
                ptypeStuff += paramTypes[i];
                if (i < (int)paramTypes.size() - 1)
                    ptypeStuff += ", ";
            }

            for (int i = 0; i < (int)paramNames.size(); i++) {
                if (paramTypes[i] == "std::string")
                    pnameStuff += "::BNM::CreateMonoString(" + paramNames[i] + ")";
                else
                    pnameStuff += paramNames[i];
                if (i < (int)paramNames.size() - 1)
                    pnameStuff += ", ";
            }

            std::string pns = std::string(Il2Cpp::il2cpp_class_get_namespace(klass));
            if (pns != "") {
                pns = pns + ".";
            }

            pns += Il2Cpp::il2cpp_class_get_name(klass);

            writer.WriteLine("static auto " + bnmMethodName + fmt::format(" = (T(*)({}))::BNM::GetExternMethod(\"{}::{}\");", ptypeStuff, pns, Il2Cpp::il2cpp_method_get_name(method)));

            writer.Write("return ");
            writer.Write(fmt::format("{}({})", bnmMethodName, pnameStuff));

            if (cppType == "std::string")
                writer.Write("->str()");

            writer.Write(";");

            writer.CloseBracket();
        } else {
            auto tFixed = retIsArray ? "::BNM::Structures::Mono::Array<T>*" : "T";
            if (retIsList) {
                tFixed = "::BNM::Structures::Mono::List<T>*";
            }
            if (paramCount > 0) {
                std::string paramNameArr = "";
                for (int i = 0; i < (int)paramCount; i++) {
                    const char* originalParamName = Il2Cpp::il2cpp_method_get_param_name(method, i);
                    paramNameArr += fmt::format("\"{}\"", originalParamName ? originalParamName : "");
                    if (i < (int)paramCount - 1)
                        paramNameArr += ", ";
                }
                writer.Write(fmt::format("static ::BNM::Method<{}> ", cppType == "std::string" ? "::BNM::Structures::Mono::String*" : tFixed) + bnmMethodName + fmt::format(" = StaticClass().GetMethod(\"{}\", {{{}}})", std::string(name), paramNameArr));
            } else {
                writer.Write(fmt::format("static ::BNM::Method<{}> ", cppType == "std::string" ? "::BNM::Structures::Mono::String*" : tFixed) + bnmMethodName + fmt::format(" = StaticClass().GetMethod(\"{}\", {})", std::string(name), std::to_string(paramCount)));
            }

            writer.WriteLine(";");


            if (!isStatic)
                writer.WriteLine(fmt::format("{}.SetInstance((BNM::IL2CPP::Il2CppObject*)this);", bnmMethodName));

            if (cppType != "void") {
                writer.Write("return ");
            }
            writer.Write(fmt::format("{}.Call(", bnmMethodName));

            for (int i = 0; i < (int)paramNames.size(); ++i) {
                if (paramTypes[i] == "std::string") {
                    writer.Write("::BNM::CreateMonoString(" + paramNames[i] + ")");
                }
                else {
                    if (Il2Cpp::il2cpp_type_is_byref(paramTypesReal[i])) {
                        writer.Write("&");
                    }
                    writer.Write(paramNames[i]);
                }
                if (i < (int)paramNames.size() - 1) {
                    writer.Write(", ");
                }
            }
            writer.Write(")");

            if (cppType == "std::string")
                writer.Write("->str()");

            writer.WriteLine(";");

            writer.CloseBracket();
        }
    }
}

void codegen::parseFields(void *klass, CppCodeWriter &writer) {
    if (!klass) return;

    void* fieldIter = nullptr;
    void* field = nullptr;

    field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter);

    while (field != nullptr) {
        const char* name = Il2Cpp::il2cpp_field_get_name(field);
        if (!name) {
            field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter);
            continue;
        }

        const void* fieldType = Il2Cpp::il2cpp_field_get_type(field);
        if (!fieldType) {
            field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter);
            continue;
        }

        const void* fieldClass = Il2Cpp::il2cpp_class_from_type(fieldType);
        if (!fieldClass) {
            field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter);
            continue;
        }

        std::string fieldClassName = Utils::FixName(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(fieldClass)));
        bool isDisplayClass = fieldClassName.find("__c__DisplayClass") != std::string::npos ||
                              fieldClassName.find("__f__AnonymousType") != std::string::npos ||
                              fieldClassName.find("$$c__DisplayClass") != std::string::npos ||
                              fieldClassName.find("$$f__AnonymousType") != std::string::npos;

        uint32_t flags = Il2Cpp::il2cpp_field_get_flags(field);

        std::string rawFieldClassName = std::string(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(fieldClass)));
        bool isArray = rawFieldClassName.ends_with("[]");
        bool isList = rawFieldClassName == "List`1";

        std::string cppType;
        if (isDisplayClass) {
            cppType = "BNM::IL2CPP::Il2CppObject*";
        } else {
            cppType = Utils::GetCppNameFromIl2CppType(const_cast<void*>(fieldClass), klass, &writer);
        }

        writer.WriteComment(std::string(Il2Cpp::il2cpp_class_get_namespace(const_cast<void*>(fieldClass))) + "." +
                            std::string(Il2Cpp::il2cpp_class_get_name(const_cast<void*>(fieldClass))) +
                            " | Offset: 0x" + std::to_string(Il2Cpp::il2cpp_field_get_offset(field)));

        bool isStatic = flags & FIELD_ATTRIBUTE_STATIC;
        std::string fixedName = Utils::FixName(name);

        writer.WriteLine("template <typename T = " + cppType + ">");
        if (isStatic) writer.Write("static ");

        if (isArray) {
            writer.Write("::BNM::Structures::Mono::Array<T>* " + fixedName + "()");
        } else if (isList) {
            writer.Write("::BNM::Structures::Mono::List<T>* " + fixedName + "()");
        } else {
            writer.Write("T " + fixedName + "()");
        }

        auto tFixed = isArray ? "::BNM::Structures::Mono::Array<T>*" : "T";
        if (isList) {
            tFixed = "::BNM::Structures::Mono::List<T>*";
        }

        writer.WriteBracket();
        writer.WriteLine(fmt::format(
                "static BNM::Field<{}> __bnm__field__ = StaticClass().GetField(\"{}\");",
                cppType == "std::string" ? "BNM::Structures::Mono::String*" : tFixed,
                name
        ));
        if (!isStatic) writer.WriteLine("__bnm__field__.SetInstance((BNM::IL2CPP::Il2CppObject*)this);");

        if (isArray) {
            writer.Write("return (::BNM::Structures::Mono::Array<T>*)__bnm__field__()");
        } else if (isList) {
            writer.Write("return (::BNM::Structures::Mono::List<T>*)__bnm__field__()");
        } else {
            writer.Write("return __bnm__field__()");
            if (cppType == "std::string")
                writer.Write("->str()");
        }
        writer.WriteLine(";");
        writer.CloseBracket();

        writer.WriteLine("template <typename T = " + cppType + ">");
        if (isStatic) writer.Write("static ");

        std::string setterParamType;
        if (isArray) {
            setterParamType = "::BNM::Structures::Mono::Array<T>*";
        } else if (isList) {
            setterParamType = "::BNM::Structures::Mono::List<T>*";
        } else {
            setterParamType = cppType;
        }

        writer.Write("void set_" + fixedName + "(" + setterParamType + " $value)");
        writer.WriteBracket();
        writer.WriteLine(fmt::format(
                "static BNM::Field<{}> __bnm__field__ = StaticClass().GetField(\"{}\");",
                cppType == "std::string" ? "BNM::Structures::Mono::String*" : tFixed,
                name
        ));
        if (!isStatic) writer.WriteLine("__bnm__field__.SetInstance((BNM::IL2CPP::Il2CppObject*)this);");

        writer.Write("__bnm__field__.Set(");
        if (cppType == "std::string")
            writer.Write("::BNM::CreateMonoString($value)");
        else
            writer.Write("$value");
        writer.WriteLine(");");
        writer.CloseBracket();

        writer.WriteLine();

        field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter);
    }
}

std::string codegen::getClassType(void* klass){
    bool isClassEnum = Il2Cpp::il2cpp_class_is_enum(klass);
    bool isClassValueType = Il2Cpp::il2cpp_class_is_valuetype(klass);
    std::string classType;
    if (isClassValueType && !isClassEnum)
        classType = "struct ";
    else if (isClassEnum)
        classType = "enum class ";
    else
        classType = "class ";
    return classType;
}

std::string codegen::forwardDeclare(void* klass, void* parentKlass, CppCodeWriter& writer) {
    if (fmt::format("{}.{}", Il2Cpp::il2cpp_class_get_namespace(parentKlass), Il2Cpp::il2cpp_class_get_name(parentKlass)) ==
        fmt::format("{}.{}", Il2Cpp::il2cpp_class_get_namespace(klass), Il2Cpp::il2cpp_class_get_name(klass))) {
        return "";
    }

    std::string ifacens = Il2Cpp::il2cpp_class_get_namespace(klass);
    std::string ifacen = Utils::FixName(Il2Cpp::il2cpp_class_get_name(klass));

    bool isDisplayClass = ifacen.find("__c__DisplayClass") != std::string::npos ||
                          ifacen.find("__f__AnonymousType") != std::string::npos ||
                          ifacen.find("$$c__DisplayClass") != std::string::npos ||
                          ifacen.find("$$f__AnonymousType") != std::string::npos;

    if (ifacens.empty()) {
        ifacens = "GlobalNamespace";
    } else if (ifacens.starts_with("UnityEngine")) {
        ifacens = "C" + ifacens;
    }

    if (isDisplayClass) {
        return "BNM::IL2CPP::Il2CppObject*";
    }

    if ((ifacens + "." + ifacen) != "System.Object") {
        writer.WriteInclude(fmt::format("SDK/Include/{}/{}.hpp", ifacens, ifacen));
    }

    Utils::replaceAll(ifacens, ".", "::");
    std::string ret = fmt::format("::{}::{}", ifacens, ifacen);
    if (ret == "::System::Object")
        ret = "::BNM::IL2CPP::Il2CppObject";
    return ret;
}

void codegen::parseClass(void* klass) {
    if (!klass) return;

    Settings::classCount++;

    const char* name = Il2Cpp::il2cpp_class_get_name(klass);
    if (!name || strcmp(name, "<Module>") == 0) return;

    const char* imageName = Il2Cpp::il2cpp_image_get_name(Il2Cpp::il2cpp_class_get_image(klass));
    auto namespaze = std::string(Il2Cpp::il2cpp_class_get_namespace(klass));
    std::string originalNamespace = namespaze;

    auto fullNameRaw = fmt::format("{}.{}", originalNamespace, name);
    if (Utils::DefaultTypeMap.contains(fullNameRaw)) {
        return;
    }

    if (namespaze == "") {
        namespaze = "GlobalNamespace";
    } else if (std::string(namespaze).starts_with("UnityEngine")) {
        namespaze = fmt::format("C{}", namespaze);
    }

    std::string fixedName = Utils::FixName(name);
    if (fixedName.starts_with("$$c__DisplayClass") || fixedName.starts_with("$$f__AnonymousType") ||
        fixedName.find("__c__DisplayClass") != std::string::npos || fixedName.find("__f__AnonymousType") != std::string::npos) {
        return;
    }


    std::string appendPath = Utils::GetDir({ namespaze + ".hpp" });

    bool isFirstWrite = createdNamespaceFiles.find(appendPath) == createdNamespaceFiles.end();
    std::string format = fmt::format("#include <SDK/Include/{}/{}.hpp>", namespaze, fixedName);

    bool needsWrite = true;

    if (!isFirstWrite) {
        std::ifstream infile(appendPath);
        if (infile) {
            std::string content((std::istreambuf_iterator<char>(infile)),
                                std::istreambuf_iterator<char>());
            needsWrite = (content.find(format) == std::string::npos);
        }
    }

    if (needsWrite) {
        std::ofstream outfile(
                appendPath,
                isFirstWrite ? std::ios::trunc : std::ios::app
        );

        if (outfile) {
            outfile << format << '\n';
            createdNamespaceFiles.insert(appendPath);
        }
    }

    std::string path = Utils::GetDir({ "Include", namespaze, fixedName + ".hpp" });
    std::ofstream ofstr(path);
    CppCodeWriter writer(ofstr);

    writer.WriteTopLine("#pragma once");
    writer.WriteInclude("BNMIncludes.hpp");
    writer.WriteLine();

    std::vector<std::string> namespaceSplit = split(namespaze, ".");

    bool isClassEnum = Il2Cpp::il2cpp_class_is_enum(klass);
    bool isClassValueType = Il2Cpp::il2cpp_class_is_valuetype(klass);

    if (isClassEnum) {
        void* fieldIter = nullptr;
        void* field = nullptr;
        while ((field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter)) != nullptr) {
            const char* fieldName = Il2Cpp::il2cpp_field_get_name(field);
            const void* fieldType = Il2Cpp::il2cpp_field_get_type(field);

            writer.WriteLine("#undef " + Utils::FixName(fieldName));
        }
    }

    if (namespaceSplit.empty()) {
        writer.Write("namespace GlobalNamespace ");
        writer.WriteBracket();
    } else {
        for (const std::string& ns : namespaceSplit) {
            writer.Write("namespace " + ns + " ");
            writer.WriteBracket();
        }
    }

    std::string classType = getClassType(klass);
    writer.Write(classType + fixedName + " ");

    if (!(isClassValueType || isClassEnum)) {
        void* parentKlass = Il2Cpp::il2cpp_class_get_parent(klass);

        if (parentKlass != nullptr) {
            const char *ns = Il2Cpp::il2cpp_class_get_namespace(parentKlass);
            const char *n = Il2Cpp::il2cpp_class_get_name(parentKlass);

            std::vector<std::string> interfaces;
            std::string sheesh = forwardDeclare(parentKlass, klass, writer);
            if (!sheesh.empty()) {
                interfaces.push_back(sheesh);
            }

            for (int i = 0; i < interfaces.size(); i++) {
                bool first = (i == 0);
                bool last = (i == interfaces.size() - 1);

                std::string baseClass = interfaces[i];
                bool isPointer = baseClass.find('*') != std::string::npos;
                
                if (isPointer) {
                    continue;
                }

                if (baseClass == "System::Object")
                    baseClass = "::BNM::IL2CPP::Il2CppObject";

                if (first)
                    writer.Write(": ");

                writer.Write(fmt::format("public {}", baseClass));

                if (!last)
                    writer.Write(", ");
            }
        }
    }

    writer.WriteBracket();

    if (classType == "class ") {
        writer.WriteLine("public:");
        writer.Write("static ::BNM::Class StaticClass() ");
        writer.WriteBracket();
        writer.WriteLine(fmt::format("static ::BNM::Class clazz = ::BNM::Class(\"{}\", \"{}\", ::BNM::Image(\"{}\"));", Il2Cpp::il2cpp_class_get_namespace(klass), Il2Cpp::il2cpp_class_get_name(klass), imageName));
        writer.WriteLine("return clazz;");
        writer.CloseBracket();

        writer.Write("static ::BNM::MonoType* BNMType() ");
        writer.WriteBracket();
        writer.WriteLine("static ::BNM::MonoType* myType = StaticClass().GetMonoType();");
        writer.WriteLine("return myType;");
        writer.CloseBracket();

        parseFields(klass, writer);
        writer.WriteLine();
        parseMethods(klass, writer);
    } else if (classType == "struct ") {
        void* fieldIter = nullptr;
        void* field = nullptr;
        while ((field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter)) != nullptr) {
            auto flags = Il2Cpp::il2cpp_field_get_flags(field);

            if (flags & FIELD_ATTRIBUTE_LITERAL) {
                continue;
            }
            if (flags & FIELD_ATTRIBUTE_STATIC) {
                continue;
            }

            const char* fieldName = Il2Cpp::il2cpp_field_get_name(field);
            const void* fieldType = Il2Cpp::il2cpp_field_get_type(field);
            auto fieldClass = Il2Cpp::il2cpp_class_from_type(fieldType);

            uint64_t staticVal = 0;
            std::ostringstream offsetStream;
            offsetStream << std::hex << Il2Cpp::il2cpp_field_get_offset(field);

            std::ostringstream rvaStream;
            rvaStream << std::hex << ((uintptr_t)Il2Cpp::il2cpp_field_get_offset(field) - mainMap->startAddress);

            writer.WriteComment("Original Type: " +
                                std::string(Il2Cpp::il2cpp_class_get_namespace(fieldClass)) + ":" +
                                std::string(Il2Cpp::il2cpp_class_get_name(fieldClass)) +
                                " | Offset 0x" + offsetStream.str());
            writer.WriteLine(Utils::GetCppNameFromIl2CppType(fieldClass, nullptr, &writer) + " " + std::string(Utils::FixName(fieldName)) + ";");
        }
    } else {
        void* fieldIter = nullptr;
        void* field = nullptr;
        while ((field = Il2Cpp::il2cpp_class_get_fields(klass, &fieldIter)) != nullptr) {
            const char* fieldName = Il2Cpp::il2cpp_field_get_name(field);
            const void* fieldType = Il2Cpp::il2cpp_field_get_type(field);

            if (strcmp(fieldName, "value__") == 0) continue;

            uint64_t staticVal = 0;
            writer.WriteLine(std::string(Utils::FixName(fieldName)) + ",");
        }
    }

    writer.CloseBracket(";");
    for (size_t i = 0; i < namespaceSplit.size(); ++i) {
        writer.CloseBracket();
    }

    // BNM::Defaults::Get setup for non-valuetype classes for generics hopefully. Structs won't work on this but who cares, I'm not perfect
    if (!isClassValueType) {
        Utils::replaceAll(namespaze, ".", "::");
        std::string typePath = fmt::format("{}::{}*", namespaze, fixedName);

        writer.WriteLine();
        writer.WriteLine("template <>");
        writer.Write("BNM::Defaults::DefaultTypeRef ");
        writer.Write("BNM::Defaults::Get<" + typePath + ">() ");
        writer.WriteBracket();
        writer.WriteLine("static BNM::Defaults::Internal::ClassType classCache = nullptr;");
        writer.WriteLine(fmt::format("if (!classCache) classCache = {}._data;", Utils::GetClassGetter(klass)));
        writer.WriteLine("return BNM::Defaults::DefaultTypeRef ({ &classCache });");
        writer.CloseBracket();
    }

    writer.WriteToFile();
    ofstr.close();
}