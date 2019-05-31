#pragma once
#include "HandleImpl.h"

namespace panda { namespace unievent { namespace backend {

struct IAsyncListener {
    virtual void handle_async () = 0;
};

struct AsyncImpl : HandleImpl {
    AsyncImpl (LoopImpl* loop, IAsyncListener* lst) : HandleImpl(loop), listener(lst) {}

    virtual void send () = 0;

    void handle_async () noexcept {
        ltry([&]{ listener->handle_async(); });
    }

    IAsyncListener* listener;
};

}}}
