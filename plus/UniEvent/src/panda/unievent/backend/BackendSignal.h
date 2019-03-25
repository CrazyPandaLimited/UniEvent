#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct ISignalListener {
    virtual void handle_signal (int signum) = 0;
};

struct BackendSignal : BackendHandle {
    BackendSignal (ISignalListener* l) : listener(l) {}

    virtual int signum () const = 0;

    virtual void start (int signum) = 0;
    virtual void once  (int signum) = 0;
    virtual void stop  ()           = 0;

    void handle_signal (int signum) noexcept {
        ltry([&]{ listener->handle_signal(signum); });
    }

    ISignalListener* listener;
};

}}}
