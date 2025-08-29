#pragma once
#include "dlfcn.h"
#include "Il2Cpp-Headers.hpp"

#define IL2CPP_FUNC(ret, name, symbolName, params) \
    inline ret (*name) params = (ret (*) params)dlsym(dlopen("libil2cpp.so", RTLD_LAZY), symbolName);

namespace Il2Cpp {
    IL2CPP_FUNC(void*, il2cpp_domain_get, symbol_il2cpp_domain_get, ());
    IL2CPP_FUNC(void*, il2cpp_thread_attach, symbol_il2cpp_thread_attach, (void*));
    IL2CPP_FUNC(const void**, il2cpp_domain_get_assemblies, symbol_il2cpp_domain_get_assemblies, (const void*, size_t*));
    IL2CPP_FUNC(const void*, il2cpp_assembly_get_image, symbol_il2cpp_assembly_get_image, (const void*));

    IL2CPP_FUNC(int, il2cpp_field_get_flags, symbol_il2cpp_field_get_flags, (void*));
    IL2CPP_FUNC(void*, il2cpp_class_get_interfaces, symbol_il2cpp_class_get_interfaces, (void *, void* *));
    IL2CPP_FUNC(const char*, il2cpp_field_get_name, symbol_il2cpp_field_get_name, (void*));
    IL2CPP_FUNC(const void*, il2cpp_field_get_type, symbol_il2cpp_field_get_type, (void*));
    IL2CPP_FUNC(void*, il2cpp_class_get_fields, symbol_il2cpp_class_get_fields, (void*, void**));
    IL2CPP_FUNC(size_t, il2cpp_field_get_offset, symbol_il2cpp_field_get_offset, (void*));
    IL2CPP_FUNC(void, il2cpp_field_static_get_value, symbol_il2cpp_field_static_get_value, (void*, void*));
    IL2CPP_FUNC(bool, il2cpp_type_is_byref, symbol_il2cpp_type_is_byref, (const void *))
    IL2CPP_FUNC(void*, il2cpp_type_get_class_or_element_class, symbol_il2cpp_type_get_class_or_element_class, (const void *));
    IL2CPP_FUNC(const void*, il2cpp_class_get_type, symbol_il2cpp_class_get_type, (void*));

    IL2CPP_FUNC(size_t, il2cpp_image_get_class_count, symbol_il2cpp_image_get_class_count, (const void*));
    IL2CPP_FUNC(const void*, il2cpp_image_get_class, symbol_il2cpp_image_get_class, (const void*, size_t));

    IL2CPP_FUNC(void*, il2cpp_class_get_methods, symbol_il2cpp_class_get_methods, (void*, void**));
    IL2CPP_FUNC(const void*, il2cpp_method_get_return_type, symbol_il2cpp_method_get_return_type, (const void*));
    IL2CPP_FUNC(const char*, il2cpp_method_get_name, symbol_il2cpp_method_get_name, (const void*));
    IL2CPP_FUNC(uint32_t, il2cpp_method_get_param_count, symbol_il2cpp_method_get_param_count, (const void*));
    IL2CPP_FUNC(const void*, il2cpp_method_get_param, symbol_il2cpp_method_get_param, (const void*, uint32_t));
    IL2CPP_FUNC(const char*, il2cpp_method_get_param_name, symbol_il2cpp_method_get_param_name, (const void*, uint32_t));
    IL2CPP_FUNC(uint32_t, il2cpp_method_get_flags, symbol_il2cpp_method_get_flags, (const void*, uint32_t*))

    IL2CPP_FUNC(bool, il2cpp_class_is_valuetype, symbol_il2cpp_class_is_valuetype, (const void*));
    IL2CPP_FUNC(bool, il2cpp_class_is_enum, symbol_il2cpp_class_is_enum, (const void*));

    IL2CPP_FUNC(void*, il2cpp_class_from_type, symbol_il2cpp_class_from_type, (const void*));
    IL2CPP_FUNC(const char*, il2cpp_class_get_name, symbol_il2cpp_class_get_name, (void*));
    IL2CPP_FUNC(const char*, il2cpp_class_get_namespace, symbol_il2cpp_class_get_namespace, (void*));
    IL2CPP_FUNC(void*, il2cpp_class_get_parent, symbol_il2cpp_class_get_parent, (void*));
    IL2CPP_FUNC(bool, il2cpp_type_is_static, symbol_il2cpp_type_is_static, (const void*));
    IL2CPP_FUNC(const char*, il2cpp_image_get_name, symbol_il2cpp_image_get_name, (const void*));
    IL2CPP_FUNC(const void*, il2cpp_class_get_image, symbol_il2cpp_class_get_image, (void*));
    IL2CPP_FUNC(void*, il2cpp_class_from_name, symbol_il2cpp_class_from_name, (const void*, const char*, const char*));

}