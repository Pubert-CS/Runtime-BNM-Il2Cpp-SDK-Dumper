#pragma once
#include "CppCodeWriter.hpp"
#include "KittyMemory/KittyInclude.hpp"

namespace codegen {
    inline void* libHandle = nullptr;

    std::string getClassType(void*);
    std::string forwardDeclare(void*, void*, CppCodeWriter&);
    std::vector<std::string> getParamTypes(void*);
    std::vector<std::string> getParamNames(void*);
    std::vector<std::string> split(std::string, std::string);
    std::string joinString(std::vector<std::string>, std::string);

    void start(void*, ProcMap);
    void parseAssembly(const void*);
    void parseClass(void*);
    void parseMethods(void*, CppCodeWriter&);
    void parseFields(void*, CppCodeWriter&);

    void parseNoThreadCheck();
}