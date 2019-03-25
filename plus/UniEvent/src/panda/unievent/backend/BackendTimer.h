#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct ITimerListener {
    virtual void handle_timer () = 0;
};

struct BackendTimer : BackendHandle {
    BackendTimer (ITimerListener* l) : listener(l) {}

    virtual void     start  (uint64_t repeat, uint64_t initial) = 0;
    virtual void     stop   () noexcept = 0;
    virtual void     again  () = 0;
    virtual uint64_t repeat () const = 0;
    virtual void     repeat (uint64_t repeat) = 0;

    void handle_timer () noexcept {
        ltry([&]{ listener->handle_timer(); });
    }

    ITimerListener* listener;
};

}}}