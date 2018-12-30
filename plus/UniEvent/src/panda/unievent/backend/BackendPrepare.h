#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct BackendPrepare : BackendHandle {
    BackendPrepare (Prepare* frontend) : frontend(frontend) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    Prepare* frontend;
};

}}}
