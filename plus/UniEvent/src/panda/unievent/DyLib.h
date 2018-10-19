#pragma once
#include "Error.h"

namespace panda { namespace unievent {

struct DyLib {
    DyLib () : opened(false) {}

    virtual void  open (const char* filename);
    virtual void* sym  (const char* name);

    virtual ~DyLib ();

private:
    uv_lib_t lib;
    bool     opened;
};

}}
