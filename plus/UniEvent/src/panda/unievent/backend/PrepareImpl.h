#pragma once
#include "HandleImpl.h"

namespace panda { namespace unievent { namespace backend {

struct IPrepareListener {
    virtual void handle_prepare () = 0;
};

struct PrepareImpl : HandleImpl {
    PrepareImpl (LoopImpl* loop, IPrepareListener* lst) : HandleImpl(loop), listener(lst) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    void handle_prepare () noexcept {
        ltry([&]{ listener->handle_prepare(); });
    }

    IPrepareListener* listener;
};

}}}
