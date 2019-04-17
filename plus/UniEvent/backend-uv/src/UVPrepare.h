#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendPrepare.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPrepare : UVHandle<BackendPrepare, uv_prepare_t> {
    UVPrepare (UVLoop* loop, IPrepareListener* lst) : UVHandle<BackendPrepare, uv_prepare_t>(loop, lst) {
        uv_prepare_init(loop->uvloop, &uvh);
    }

    void start () override {
        uv_prepare_start(&uvh, [](uv_prepare_t* p) {
            get_handle<UVPrepare*>(p)->handle_prepare();
        });
    }

    void stop () override {
        uv_prepare_stop(&uvh);
    }
};

}}}}
