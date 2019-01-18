#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct BackendAsync : BackendHandle {
    BackendAsync (Async* frontend) : frontend(frontend) {}

    virtual void send () = 0;

    Async* frontend;
};

}}}
