#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct ICheckListener {
    virtual void on_check () = 0;
};

struct BackendCheck : BackendHandle {
    BackendCheck (ICheckListener* l) : listener(l) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    ICheckListener* listener;
};

}}}
