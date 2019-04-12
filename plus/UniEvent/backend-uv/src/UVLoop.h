#pragma once
#include "inc.h"
#include "UVUdp.h"
#include "UVIdle.h"
#include "UVPoll.h"
#include "UVPipe.h"
#include "UVTimer.h"
#include "UVCheck.h"
#include "UVAsync.h"
#include "UVSignal.h"
#include "UVDelayer.h"
#include "UVPrepare.h"
#include <panda/unievent/backend/BackendLoop.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVLoop : BackendLoop {
    UVLoop (Type type) : _delayer(this) {
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
        _delayer.destroy();
        run(RunMode::DEFAULT); // finish all closing handles
        run(RunMode::DEFAULT); // finish all closing handles
        int err = uv_loop_close(_uvloop);
        assert(!err); // unievent should have closed all handles
    }

    uint64_t now         () const override { return uv_now(_uvloop); }
    void     update_time ()       override { uv_update_time(_uvloop); }
    bool     alive       () const override { return uv_loop_alive(_uvloop) != 0; }

    bool _run (RunMode mode) override {
        _uvloop->stop_flag = 0; // fix bug when UV immediately exits run() if stop() was called before run()
        switch (mode) {
            case RunMode::DEFAULT: return uv_run(_uvloop, UV_RUN_DEFAULT);
            case RunMode::ONCE   : return uv_run(_uvloop, UV_RUN_ONCE);
            case RunMode::NOWAIT : return uv_run(_uvloop, UV_RUN_NOWAIT);
        }
        assert(0);
    }

    void stop () override {
        uv_stop(_uvloop);
    }

    bool stopped () const override {
        return _uvloop->stop_flag;
    }

    void handle_fork () override {
        int err = uv_loop_fork(_uvloop);
        if (err) throw uvx_code_error(err);
    }

    BackendTimer*   new_timer     (ITimerListener* l)             override { return new UVTimer(_uvloop, l); }
    BackendPrepare* new_prepare   (IPrepareListener* l)           override { return new UVPrepare(_uvloop, l); }
    BackendCheck*   new_check     (ICheckListener* l)             override { return new UVCheck(_uvloop, l); }
    BackendIdle*    new_idle      (IIdleListener* l)              override { return new UVIdle(_uvloop, l); }
    BackendAsync*   new_async     (IAsyncListener* l)             override { return new UVAsync(_uvloop, l); }
    BackendSignal*  new_signal    (ISignalListener* l)            override { return new UVSignal(_uvloop, l); }
    BackendPoll*    new_poll_sock (IPollListener* l, sock_t sock) override { return new UVPoll(_uvloop, l, sock); }
    BackendPoll*    new_poll_fd   (IPollListener* l, int fd)      override { return new UVPoll(_uvloop, l, fd, nullptr); }
    BackendUdp*     new_udp       (IUdpListener* l, int domain)   override { return new UVUdp(_uvloop, l, domain); }
    BackendPipe*    new_pipe      (IStreamListener* l, bool ipc)  override { return new UVPipe(_uvloop, l, ipc); }

    BackendSendRequest*     new_send_request     (ISendListener* l)     override { return new UVSendRequest(l); }
    BackendConnectRequest*  new_connect_request  (IConnectListener* l)  override { return new UVConnectRequest(l); }
    BackendShutdownRequest* new_shutdown_request (IShutdownListener* l) override { return new UVShutdownRequest(l); }

    uint64_t delay        (const delayed_fn& f, const iptr<Refcnt>& guard = {}) { return _delayer.add(f, guard); }
    void     cancel_delay (uint64_t id) noexcept                                { _delayer.cancel(id); }

private:
    uv_loop_t  _uvloop_body;
    uv_loop_t* _uvloop;
    UVDelayer  _delayer;
};

}}}}
