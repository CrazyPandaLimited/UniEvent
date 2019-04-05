#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendIdle.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVIdle : UVHandle<BackendIdle, uv_idle_t> {
    UVIdle (uv_loop_t* loop, IIdleListener* lst) : UVHandle<BackendIdle, uv_idle_t>(lst) {
        uv_idle_init(loop, &uvh);
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
