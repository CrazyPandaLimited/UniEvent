#pragma once
#include "types.h"
#include "forward.h"
#include <exception>
#include <panda/function.h>

namespace panda { namespace unievent { namespace backend {

struct BackendLoop {
    using delayed_fn = function<void()>;
    enum class Type    { LOCAL, GLOBAL, DEFAULT };
    enum class RunMode { DEFAULT, ONCE, NOWAIT };

    std::exception_ptr _exception;

    BackendLoop () {}

    virtual uint64_t now         () const = 0;
    virtual void     update_time () = 0;
    virtual bool     alive       () const = 0;

    bool run (RunMode mode) {
        bool ret = _run(mode);
        if (_exception) std::rethrow_exception(std::move(_exception));
        return ret;
    }

    virtual bool _run        (RunMode) = 0; // returns false if there are no more active handles
    virtual void stop        () = 0;
    virtual bool stopped     () const = 0;
    virtual void handle_fork () = 0;

    virtual BackendTimer*   new_timer     (ITimerListener*)             = 0;
    virtual BackendPrepare* new_prepare   (IPrepareListener*)           = 0;
    virtual BackendCheck*   new_check     (ICheckListener*)             = 0;
    virtual BackendIdle*    new_idle      (IIdleListener*)              = 0;
    virtual BackendAsync*   new_async     (IAsyncListener*)             = 0;
    virtual BackendSignal*  new_signal    (ISignalListener*)            = 0;
    virtual BackendPoll*    new_poll_sock (IPollListener*, sock_t sock) = 0;
    virtual BackendPoll*    new_poll_fd   (IPollListener*, int fd)      = 0;
    virtual BackendUdp*     new_udp       (IUdpListener*, int domain)   = 0;
    virtual BackendPipe*    new_pipe      (IStreamListener*, bool ipc)  = 0;

    virtual BackendSendRequest* new_send_request (ISendListener*) = 0;

    virtual uint64_t delay        (const delayed_fn& f, const iptr<Refcnt>& guard = {}) = 0;
    virtual void     cancel_delay (uint64_t id) noexcept = 0;

//    template <class Func>
//    void ltry (Func&& f) {
//        try { f(); }
//        catch (...) { capture_exception(); }
//    }

    void capture_exception ();

    virtual ~BackendLoop () {}
};

}}}
