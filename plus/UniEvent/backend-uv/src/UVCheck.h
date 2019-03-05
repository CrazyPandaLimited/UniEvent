#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendCheck.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVCheck : UVHandle<BackendCheck> {
    UVCheck (uv_loop_t* loop, ICheckListener* lst) : UVHandle<BackendCheck>(lst) {
        uv_check_init(loop, &uvh);
        _init(&uvh);
    }

    void start () override {
        uv_check_start(&uvh, _call);
    }

    void stop () override {
        uv_check_stop(&uvh);
    }

private:
    uv_check_t uvh;

    static void _call (uv_check_t* p) {
        auto h = get_handle<UVCheck*>(p);
        if (h->listener) h->listener->on_check();
    }
};

}}}}
