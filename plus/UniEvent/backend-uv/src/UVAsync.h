#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendAsync.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVAsync : UVHandle<BackendAsync, uv_async_t> {
    UVAsync (uv_loop_t* loop, IAsyncListener* lst) : UVHandle<BackendAsync, uv_async_t>(lst) {
        int err = uv_async_init(loop, &uvh, [](uv_async_t* p){
            get_handle<UVAsync*>(p)->handle_async();
        });
        if (err) throw uvx_code_error(err);
    }

    void send () override {
        int err = uv_async_send(&uvh);
        if (err) throw uvx_code_error(err);
    }
};

}}}}
