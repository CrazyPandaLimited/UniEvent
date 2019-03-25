#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IAsyncListener {
    virtual void handle_async () = 0;
};

struct BackendAsync : BackendHandle {
    BackendAsync (IAsyncListener* l) : listener(l) {}

    virtual void send () = 0;

    void handle_async () noexcept {
        ltry([&]{ listener->handle_async(); });
    }

    IAsyncListener* listener;
};

}}}
