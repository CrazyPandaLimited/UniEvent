#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendIdle.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVIdle : UVHandle<BackendIdle, uv_idle_t> {
    UVIdle (UVLoop* loop, IIdleListener* lst) : UVHandle<BackendIdle, uv_idle_t>(loop, lst) {
        uv_idle_init(loop->uvloop, &uvh);
    }

    void start () override {
        uv_idle_start(&uvh, [](uv_idle_t* p) {
            get_handle<UVIdle*>(p)->handle_idle();
        });
    }

    void stop () override {
        uv_idle_stop(&uvh);
    }
};

}}}}
