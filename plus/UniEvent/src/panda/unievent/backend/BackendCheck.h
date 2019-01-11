#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct BackendCheck : BackendHandle {
    BackendCheck (Check* frontend) : frontend(frontend) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    Check* frontend;
};

}}}
