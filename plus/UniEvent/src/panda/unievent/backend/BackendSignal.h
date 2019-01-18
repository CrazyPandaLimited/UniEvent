#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct BackendSignal : BackendHandle {
    BackendSignal (Signal* frontend) : frontend(frontend) {}

    virtual int signum () const = 0;

    virtual void start (int signum) = 0;
    virtual void once  (int signum) = 0;
    virtual void stop  ()           = 0;

    Signal* frontend;
};

}}}
