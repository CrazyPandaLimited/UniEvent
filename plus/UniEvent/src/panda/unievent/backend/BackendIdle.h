#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct BackendIdle : BackendHandle {
    BackendIdle (Idle* frontend) : frontend(frontend) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    Idle* frontend;
};

}}}
