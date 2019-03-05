#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendTimer.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVTimer : UVHandle<BackendTimer> {
    UVTimer (uv_loop_t* loop, ITimerListener* lst) : UVHandle<BackendTimer>(lst) {
        uv_timer_init(loop, &uvh);
        _init(&uvh);
    }

    void start (uint64_t repeat, uint64_t initial) override {
        uv_timer_start(&uvh, _call, initial, repeat);
    }

    void stop () override {
        uv_timer_stop(&uvh);
    }

    void again () override {
        int err = uv_timer_again(&uvh);
        if (err) throw uvx_code_error(err);
    }

    uint64_t repeat () const override {
        return uv_timer_get_repeat(&uvh);
    }

    void repeat (uint64_t repeat) override {
        uv_timer_set_repeat(&uvh, repeat);
    }

private:
    uv_timer_t uvh;

    static void _call (uv_timer_t* p) {
        auto h = get_handle<UVTimer*>(p);
        if (h->listener) h->listener->on_timer();
    }
};

}}}}
