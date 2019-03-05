#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct ISignalListener {
    virtual void on_signal (int signum) = 0;
};

struct BackendSignal : BackendHandle {
    BackendSignal (ISignalListener* l) : listener(l) {}

    virtual int signum () const = 0;

    virtual void start (int signum) = 0;
    virtual void once  (int signum) = 0;
    virtual void stop  ()           = 0;

    ISignalListener* listener;
};

}}}
