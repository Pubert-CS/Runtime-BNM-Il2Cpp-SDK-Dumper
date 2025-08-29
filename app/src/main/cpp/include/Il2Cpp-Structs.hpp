#pragma once
#include "stdint.h"

struct Il2CppType;
struct Il2CppClass;
struct Il2CppGenericClass;
struct Il2CppGenericContext;
struct Il2CppGenericInst;

struct Il2CppGenericInst {
    uint32_t type_argc;
    const Il2CppType **type_argv;
};

struct Il2CppGenericContext {
    /* The instantiation corresponding to the class generic parameters */
    const Il2CppGenericInst *class_inst;
    /* The instantiation corresponding to the method generic parameters */
    const Il2CppGenericInst *method_inst;
};


struct Il2CppGenericClass
{
    int32_t typeDefinitionIndex;	/* the generic type definition */
    Il2CppGenericContext context;	/* a context that contains the type instantiation doesn't contain any method instantiation */
    Il2CppClass *cached_class;	/* if present, the Il2CppClass corresponding to the instantiation.  */
};

struct Il2CppType {
    union {
        // We have this dummy field first because pre C99 compilers (MSVC) can only initializer the first value in a union.
        void* dummy;
        int32_t klassIndex; /* for VALUETYPE and CLASS */
        const Il2CppType *type;   /* for PTR and SZARRAY */
        void *array; /* for ARRAY */
        //MonoMethodSignature *method;
        int32_t genericParameterIndex; /* for VAR and MVAR */
        Il2CppGenericClass *generic_class; /* for GENERICINST */
    } data;
};

struct Il2CppClass {
    // The following fields are always valid for a Il2CppClass structure
    const void *image;
    void *gc_desc;
    const char *name;
    const char *namespaze;
    const Il2CppType *byval_arg;
    const Il2CppType *this_arg;
    Il2CppClass *element_class;
    Il2CppClass *castClass;
    Il2CppClass *declaringType;
    Il2CppClass *parent;
    Il2CppGenericClass *generic_class;
    const void *typeDefinition; // non-NULL for Il2CppClass's constructed from type defintions
// End always valid fields
};