#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendAsync.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVAsync : UVHandle<BackendAsync> {
    UVAsync (uv_loop_t* loop, IAsyncListener* lst) : UVHandle<BackendAsync>(lst) {
        int err = uv_async_init(loop, &uvh, _call);
        if (err) throw uvx_code_error(err);
        _init(&uvh);
    }

    void send () override {
        int err = uv_async_send(&uvh);
        if (err) throw uvx_code_error(err);
    }

private:
    uv_async_t uvh;

    static void _call (uv_async_t* p) {
        auto h = get_handle<UVAsync*>(p);
        if (h->listener) h->listener->on_async();
    }
};

}}}}
