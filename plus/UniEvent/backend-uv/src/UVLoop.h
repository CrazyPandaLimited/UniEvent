#pragma once
#include "inc.h"
#include "UVTimer.h"
#include "UVPrepare.h"
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

    uint64_t now         () const override { return uv_now(_uvloop); }
    void     update_time ()       override { uv_update_time(_uvloop); }
    bool     alive       () const override { return uv_loop_alive(_uvloop) != 0; }

    bool run        () override { return uv_run(_uvloop, UV_RUN_DEFAULT); }
    bool run_once   () override { return uv_run(_uvloop, UV_RUN_ONCE); }
    bool run_nowait () override { return uv_run(_uvloop, UV_RUN_NOWAIT); }
    void stop       () override { uv_stop(_uvloop); }

    void handle_fork () override {
        int err = uv_loop_fork(_uvloop);
        if (err) throw uvx_code_error(err);
    }

    BackendTimer*   new_timer   (Timer*   frontend) override { return new UVTimer  (_uvloop, frontend); }
    BackendPrepare* new_prepare (Prepare* frontend) override { return new UVPrepare(_uvloop, frontend); }

private:
    uv_loop_t  _uvloop_body;
    uv_loop_t* _uvloop;
};

}}}}
