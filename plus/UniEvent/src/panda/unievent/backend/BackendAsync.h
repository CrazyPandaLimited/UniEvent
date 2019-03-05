#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IAsyncListener {
    virtual void on_async () = 0;
};

struct BackendAsync : BackendHandle {
    BackendAsync (IAsyncListener* l) : listener(l) {}

    virtual void send () = 0;

    IAsyncListener* listener;
};

}}}
