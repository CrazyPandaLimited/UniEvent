#pragma once
#include "inc.h"
#include "UVIdle.h"
#include <panda/unievent/backend/BackendTick.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

// on UV, idle listener always called once per iteration (regardless of whether there were other events)

struct UVTick : UVHandle<BackendTick> {
    UVTick (uv_loop_t* loop, ITickListener* lst) : UVHandle<BackendTick>(lst) {
        uv_idle_init(loop, &uvh);
        _init(&uvh);
    }

    void start () override {
        uv_idle_start(&uvh, _call);
    }

    void stop () override {
        uv_idle_stop(&uvh);
    }

protected:
    uv_idle_t uvh;

    static void _call (uv_idle_t* p) {
        auto h = get_handle<UVTick*>(p);
        if (h->listener) h->listener->on_tick();
    }
};

}}}}
