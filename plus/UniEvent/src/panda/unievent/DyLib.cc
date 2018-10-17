#include "DyLib.h"
using namespace panda::unievent;

#define _DYLIB_CLOSE_       \
    if (!opened) return;    \
    opened = false;         \
    uv_dlclose(&lib);

void DyLib::open (const char* filename) {
    _DYLIB_CLOSE_;
    int err = uv_dlopen(filename, &lib);
    if (err) throw DyLibError(err, &lib);
    opened = true;
}

void* DyLib::sym (const char* name) {
    void* ret;
    int err = uv_dlsym(&lib, name, &ret);
    if (err) throw DyLibError(err, &lib);
    return ret;
}

DyLib::~DyLib () {
    _DYLIB_CLOSE_;
}
