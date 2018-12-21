#pragma once
#include "inc.h"
#include "UVTimer.h"
#include <panda/unievent/backend/BackendLoop.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVLoop : BackendLoop {
    UVLoop (Loop* frontend, Type type) : BackendLoop(frontend) {
        switch (type) {
            case Type::GLOBAL:
                _uvloop = uv_default_loop();
                if (!_uvloop) throw Error("[UVLoop] uv_default_loop() couldn't create a loop");
                break;
            case Type::LOCAL:
            case Type::DEFAULT:
                _uvloop = &_uvloop_body;
                int err = uv_loop_init(_uvloop);
                if (err) throw uvx_code_error(err);
        }
        _uvloop->data = this;
    }

    ~UVLoop () {
        run_nowait(); // finish all closing handles
        int err = uv_loop_close(_uvloop);
        assert(!err); // unievent should have closed all handles
    }

    int  run        () override { return uv_run(_uvloop, UV_RUN_DEFAULT); }
    int  run_once   () override { return uv_run(_uvloop, UV_RUN_ONCE); }
    int  run_nowait () override { return uv_run(_uvloop, UV_RUN_NOWAIT); }
    void stop       () override { uv_stop(_uvloop); }

    void handle_fork () override {
        int err = uv_loop_fork(_uvloop);
        if (err) throw uvx_code_error(err);
    }

    BackendTimer* new_timer (Timer* frontend) override { return new UVTimer(_uvloop, frontend); }

private:
    uv_loop_t  _uvloop_body;
    uv_loop_t* _uvloop;
};

}}}}
