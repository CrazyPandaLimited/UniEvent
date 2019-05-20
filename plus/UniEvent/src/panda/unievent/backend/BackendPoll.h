#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IPollListener {
    virtual void handle_poll (int events, const CodeError& err) = 0;
};

struct BackendPoll : BackendHandle {
    BackendPoll (BackendLoop*, IPollListener* lst) : BackendHandle(loop), listener(lst) {}

    virtual optional<fh_t> fileno () const = 0;

    virtual void start (int events) = 0;
    virtual void stop  ()           = 0;

    void handle_poll (int events, const CodeError& err) noexcept {
        ltry([&]{ listener->handle_poll(events, err); });
    }

    IPollListener* listener;
};

}}}
