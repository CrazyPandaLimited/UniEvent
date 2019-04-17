#pragma once
#include "UVHandle.h"
#include <panda/unievent/backend/BackendTimer.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVTimer : UVHandle<BackendTimer, uv_timer_t> {
    UVTimer (UVLoop* loop, ITimerListener* lst) : UVHandle<BackendTimer, uv_timer_t>(loop, lst) {
        uv_timer_init(loop->uvloop, &uvh);
    }

    void start (uint64_t repeat, uint64_t initial) override {
        uv_timer_start(&uvh, _call, initial, repeat);
    }

    void stop () noexcept override {
        uv_timer_stop(&uvh);
    }

    void again () override {
        uvx_strict(uv_timer_again(&uvh));
    }

    uint64_t repeat () const override {
        return uv_timer_get_repeat(&uvh);
    }

    void repeat (uint64_t repeat) override {
        uv_timer_set_repeat(&uvh, repeat);
    }

private:
    static void _call (uv_timer_t* p) {
        get_handle<UVTimer*>(p)->handle_timer();
    }
};

}}}}
