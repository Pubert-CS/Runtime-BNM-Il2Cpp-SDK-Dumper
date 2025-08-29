#include <jni.h>
#include <string>
#include "dlfcn.h"
#include "iostream"
#include "unistd.h"
#include "macros.hpp"
#include "thread"
#include "SDK/codegen.hpp"

#include "KittyMemory/KittyInclude.hpp"

static bool didOnce = false;

__attribute__ ((constructor))
void lib_main() {
    if (didOnce) return;
    didOnce = true;

    LOGI("Loading %s", libFile);
    std::thread([] {
        ProcMap map;
        do {
            map = KittyMemory::getElfBaseMap("libil2cpp.so");
            sleep(1);
        } while (!map.isValidELF() || !map.isValid());

        void* handle = dlopen(libFile, RTLD_LAZY);
        codegen::start(handle, map);
    }).detach();

    return;
}