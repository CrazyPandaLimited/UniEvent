#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendPrepare.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPrepare : UVHandle<BackendPrepare> {
    UVPrepare (uv_loop_t* loop, IPrepareListener* lst) : UVHandle<BackendPrepare>(lst) {
        uv_prepare_init(loop, &uvh);
        _init(&uvh);
    }

    void start () override {
        uv_prepare_start(&uvh, _call);
    }

    void stop () override {
        uv_prepare_stop(&uvh);
    }

private:
    uv_prepare_t uvh;

    static void _call (uv_prepare_t* p) {
        auto h = get_handle<UVPrepare*>(p);
        if (h->listener) h->listener->on_prepare();
    }
};

}}}}
