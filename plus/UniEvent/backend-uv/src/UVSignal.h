#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/Signal.h>
#include <panda/unievent/backend/BackendSignal.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVSignal : UVHandle<BackendSignal> {
    UVSignal (uv_loop_t* loop, Signal* frontend) : UVHandle<BackendSignal>(frontend) {
        int err = uv_signal_init(loop, &uvh);
        if (err) throw uvx_code_error(err);
        _init(&uvh);
    }

    int signum () const override { return uvh.signum; }

    void start (int signum) override {
        int err = uv_signal_start(&uvh, _call, signum);
        if (err) throw uvx_code_error(err);
    }

    void once (int signum) override {
        int err = uv_signal_start_oneshot(&uvh, _call, signum);
        if (err) throw uvx_code_error(err);
    }

    void stop () override {
        int err = uv_signal_stop(&uvh);
        if (err) throw uvx_code_error(err);
    }

private:
    uv_signal_t uvh;

    static void _call (uv_signal_t* p, int signum) {
        auto h = get_handle<UVSignal*>(p);
        if (h->frontend) h->frontend->call_now(signum);
    }
};

}}}}
