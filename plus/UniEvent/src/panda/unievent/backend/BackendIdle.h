#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IIdleListener {
    virtual void handle_idle () = 0;
};

struct BackendIdle : BackendHandle {
    BackendIdle (IIdleListener* l) : listener(l) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    void handle_idle () noexcept {
        ltry([&]{ listener->handle_idle(); });
    }

    IIdleListener* listener;
};

}}}
