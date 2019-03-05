#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IPollListener {
    virtual void on_poll (int events, const CodeError* err) = 0;
};

struct BackendPoll : BackendHandle {
    BackendPoll (IPollListener* l) : listener(l) {}

    virtual void start (int events) = 0;
    virtual void stop  ()           = 0;

    IPollListener* listener;
};

}}}
