#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/Check.h>
#include <panda/unievent/backend/BackendCheck.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVCheck : UVHandle<BackendCheck> {
    UVCheck (uv_loop_t* loop, Check* frontend) : UVHandle<BackendCheck>(frontend) {
        uv_check_init(loop, &uvh);
        _init(&uvh);
    }

    void start () {
        uv_check_start(&uvh, _call);
    }

    void stop () {
        uv_check_stop(&uvh);
    }

private:
    uv_check_t uvh;

    static void _call (uv_check_t* p) {
        auto h = get_handle<UVCheck*>(p);
        if (h->frontend) h->frontend->call_now();
    }
};

}}}}
