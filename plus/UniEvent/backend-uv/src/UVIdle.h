#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/Idle.h>
#include <panda/unievent/backend/BackendIdle.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVIdle : UVHandle<BackendIdle> {
    UVIdle (uv_loop_t* loop, Idle* frontend) : UVHandle<BackendIdle>(frontend) {
        uv_idle_init(loop, &uvh);
        _init(&uvh);
    }

    void start () override {
        uv_idle_start(&uvh, _call);
    }

    void stop () override {
        uv_idle_stop(&uvh);
    }

private:
    uv_idle_t uvh;

    static void _call (uv_idle_t* p) {
        auto h = get_handle<UVIdle*>(p);
        if (h->frontend) h->frontend->call_now();
    }
};

}}}}
