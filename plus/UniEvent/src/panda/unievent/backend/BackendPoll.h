#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct BackendPoll : BackendHandle {
    BackendPoll (Poll* frontend) : frontend(frontend) {}

    virtual void start (int events) = 0;
    virtual void stop  ()           = 0;

    Poll* frontend;
};

}}}
