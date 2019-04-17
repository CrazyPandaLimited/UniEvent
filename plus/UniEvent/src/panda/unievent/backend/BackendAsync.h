#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IAsyncListener {
    virtual void handle_async () = 0;
};

struct BackendAsync : BackendHandle {
    BackendAsync (BackendLoop* loop, IAsyncListener* lst) : BackendHandle(loop), listener(lst) {}

    virtual void send () = 0;

    void handle_async () noexcept {
        ltry([&]{ listener->handle_async(); });
    }

    IAsyncListener* listener;
};

}}}
