#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct ITickListener {
    virtual void on_tick () = 0;
};

struct BackendTick : BackendHandle {
    BackendTick (ITickListener* l) : listener(l) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    ITickListener* listener;
};

}}}
