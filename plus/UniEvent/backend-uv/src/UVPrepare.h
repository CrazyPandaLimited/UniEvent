#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/Prepare.h>
#include <panda/unievent/backend/BackendPrepare.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPrepare : UVHandle<BackendPrepare> {
    UVPrepare (uv_loop_t* loop, Prepare* frontend) : UVHandle<BackendPrepare>(frontend) {
        uv_prepare_init(loop, &uvh);
        _init(&uvh);
    }

    void start () {
        uv_prepare_start(&uvh, uvx_on_prepare);
    }

    void stop () {
        uv_prepare_stop(&uvh);
    }

private:
    uv_prepare_t uvh;

    static void uvx_on_prepare (uv_prepare_t* p) {
        auto h = get_handle<UVPrepare*>(p);
        if (h->frontend) h->frontend->call_on_prepare();
    }
};

}}}}
