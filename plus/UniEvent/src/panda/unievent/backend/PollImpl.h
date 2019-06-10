#pragma once
#include "HandleImpl.h"

namespace panda { namespace unievent { namespace backend {

struct IPollListener {
    virtual void handle_poll (int events, const CodeError& err) = 0;
};

struct PollImpl : HandleImpl {
    PollImpl (LoopImpl* loop, IPollListener* lst) : HandleImpl(loop), listener(lst) {}

    virtual optional<fh_t> fileno () const = 0;

    virtual void start (int events) = 0;
    virtual void stop  ()           = 0;

    void handle_poll (int events, const CodeError& err) noexcept {
        ltry([&]{ listener->handle_poll(events, err); });
    }

    IPollListener* listener;
};

}}}
