#pragma once
#include "inc.h"
#include "UVIdle.h"
#include "UVTick.h"
#include "UVPoll.h"
#include "UVTimer.h"
#include "UVCheck.h"
#include "UVAsync.h"
#include "UVSignal.h"
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
        run(); // finish all closing handles
        run(); // finish all closing handles
        int err = uv_loop_close(_uvloop);
        assert(!err); // unievent should have closed all handles
    }

    uint64_t now         () const override { return uv_now(_uvloop); }
    void     update_time ()       override { uv_update_time(_uvloop); }
    bool     alive       () const override { return uv_loop_alive(_uvloop) != 0; }

    bool run        () override { return _run(UV_RUN_DEFAULT); }
    bool run_once   () override { return _run(UV_RUN_ONCE); }
    bool run_nowait () override { return _run(UV_RUN_NOWAIT); }
    void stop       () override { uv_stop(_uvloop); }

    void handle_fork () override {
        int err = uv_loop_fork(_uvloop);
        if (err) throw uvx_code_error(err);
    }

    BackendTimer*   new_timer  (ITimerListener*   l) override { return new UVTimer  (_uvloop, l); }
    BackendPrepare* new_prepare(IPrepareListener* l) override { return new UVPrepare(_uvloop, l); }
    BackendCheck*   new_check  (ICheckListener*   l) override { return new UVCheck  (_uvloop, l); }
    BackendIdle*    new_idle   (IIdleListener*    l) override { return new UVIdle   (_uvloop, l); }
    BackendAsync*   new_async  (IAsyncListener*   l) override { return new UVAsync  (_uvloop, l); }
    BackendSignal*  new_signal (ISignalListener*  l) override { return new UVSignal (_uvloop, l); }
    BackendTick*    new_tick   (ITickListener*    l) override { return new UVTick   (_uvloop, l); }

    BackendPoll* new_poll_sock (IPollListener* l, sock_t sock) override { return new UVPoll(_uvloop, l, sock); }
    BackendPoll* new_poll_fd   (IPollListener* l, int    fd  ) override { return new UVPoll(_uvloop, l, fd, nullptr); }

private:
    uv_loop_t  _uvloop_body;
    uv_loop_t* _uvloop;

    bool _run (uv_run_mode mode) {
        _uvloop->stop_flag = 0;
        return uv_run(_uvloop, mode);
    }
};

}}}}
