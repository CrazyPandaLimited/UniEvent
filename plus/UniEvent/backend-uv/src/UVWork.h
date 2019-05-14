#pragma once
#include "inc.h"
#include <panda/unievent/backend/BackendWork.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVWork : BackendWork {
    bool      active;
    uv_work_t uvr;

    UVWork (UVLoop* loop, IWorkListener* lst) : BackendWork(loop, lst), active() {
        uvr.loop = loop->uvloop;
        uvr.data = this;
    }

    void queue () override {
        auto err = uv_queue_work(uvr.loop, &uvr, on_work, on_after_work);
        if (!err) active = true;
        uvx_strict(err);
    }

    void destroy () noexcept override {
        if (active) {
            uvr.after_work_cb = [](uv_work_t* p, int) { delete get(p); };
            auto err = uv_cancel((uv_req_t*)&uvr);
            assert(!err || err == UV_EBUSY);
        }
        else delete this;
    }

private:
    static UVWork* get (uv_work_t* p) { return static_cast<UVWork*>(p->data); }

    static void on_work (uv_work_t* p) {
        get(p)->handle_work();
    }

    static void on_after_work (uv_work_t* p, int status) {
        auto w = get(p);
        w->active = false;
        w->handle_after_work(uvx_ce(status));
    }
};

}}}}
