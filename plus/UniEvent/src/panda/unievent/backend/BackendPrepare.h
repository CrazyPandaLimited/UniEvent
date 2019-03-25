#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IPrepareListener {
    virtual void handle_prepare () = 0;
};

struct BackendPrepare : BackendHandle {
    BackendPrepare (IPrepareListener* l) : listener(l) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    void handle_prepare () noexcept {
        ltry([&]{ listener->handle_prepare(); });
    }

    IPrepareListener* listener;
};

}}}
