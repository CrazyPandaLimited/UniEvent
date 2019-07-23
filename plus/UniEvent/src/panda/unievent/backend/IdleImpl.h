#pragma once
#include "HandleImpl.h"

namespace panda { namespace unievent { namespace backend {

struct IIdleListener {
    virtual void handle_idle () = 0;
};

struct IdleImpl : HandleImpl {
    IdleImpl (LoopImpl* loop, IIdleListener* lst) : HandleImpl(loop), listener(lst) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    void handle_idle () noexcept {
        ltry([&]{ listener->handle_idle(); });
    }

    IIdleListener* listener;
};

}}}
