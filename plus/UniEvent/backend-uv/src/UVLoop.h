#pragma once
#include "inc.h"
#include "UVDelayer.h"
#include <panda/unievent/backend/BackendLoop.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVLoop : BackendLoop {
    uv_loop_t* uvloop;

    UVLoop (Type type) : _delayer(this) {
        switch (type) {
            case Type::GLOBAL:
                uvloop = uv_default_loop();
                if (!uvloop) throw Error("[UVLoop] uv_default_loop() couldn't create a loop");
                break;
            case Type::LOCAL:
            case Type::DEFAULT:
                uvloop = &_uvloop_body;
                int err = uv_loop_init(uvloop);
                if (err) throw uvx_code_error(err);
        }
        uvloop->data = this;
    }

    ~UVLoop () {
        _delayer.destroy();
        run(RunMode::DEFAULT); // finish all closing handles
        run(RunMode::DEFAULT); // finish all closing handles
        int err = uv_loop_close(uvloop);
        assert(!err); // unievent should have closed all handles
    }

    uint64_t now         () const override { return uv_now(uvloop); }
    void     update_time ()       override { uv_update_time(uvloop); }
    bool     alive       () const override { return uv_loop_alive(uvloop) != 0; }

    bool _run (RunMode mode) override {
        uvloop->stop_flag = 0; // fix bug when UV immediately exits run() if stop() was called before run()
        switch (mode) {
            case RunMode::DEFAULT: return uv_run(uvloop, UV_RUN_DEFAULT);
            case RunMode::ONCE   : return uv_run(uvloop, UV_RUN_ONCE);
            case RunMode::NOWAIT : return uv_run(uvloop, UV_RUN_NOWAIT);
        }
        assert(0);
    }

    void stop () override {
        uv_stop(uvloop);
    }

    bool stopped () const override {
        return uvloop->stop_flag;
    }

    void handle_fork () override {
        int err = uv_loop_fork(uvloop);
        if (err) throw uvx_code_error(err);
    }

    BackendTimer*   new_timer     (ITimerListener* l)             override;
    BackendPrepare* new_prepare   (IPrepareListener* l)           override;
    BackendCheck*   new_check     (ICheckListener* l)             override;
    BackendIdle*    new_idle      (IIdleListener* l)              override;
    BackendAsync*   new_async     (IAsyncListener* l)             override;
    BackendSignal*  new_signal    (ISignalListener* l)            override;
    BackendPoll*    new_poll_sock (IPollListener* l, sock_t sock) override;
    BackendPoll*    new_poll_fd   (IPollListener* l, int fd)      override;
    BackendUdp*     new_udp       (IUdpListener* l, int domain)   override;
    BackendPipe*    new_pipe      (IStreamListener* l, bool ipc)  override;

    uint64_t delay        (const delayed_fn& f, const iptr<Refcnt>& guard = {}) { return _delayer.add(f, guard); }
    void     cancel_delay (uint64_t id) noexcept                                { _delayer.cancel(id); }

private:
    uv_loop_t _uvloop_body;
    UVDelayer _delayer;
};

}}}}
