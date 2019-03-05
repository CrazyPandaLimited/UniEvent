#pragma once
#include "../inc.h"
#include "forward.h"

namespace panda { namespace unievent { namespace backend {

struct BackendLoop {
    enum class Type { LOCAL, GLOBAL, DEFAULT };

    BackendLoop (Loop* frontend) : _frontend(frontend) {}

    Loop* frontend () const { return _frontend; }

    virtual uint64_t now         () const = 0;
    virtual void     update_time () = 0;
    virtual bool     alive       () const = 0;

    virtual bool run         () = 0; // returns false if there are no more active handles
    virtual bool run_once    () = 0; // -=-
    virtual bool run_nowait  () = 0; // -=-
    virtual void stop        () = 0;
    virtual void handle_fork () = 0;

    virtual BackendTimer*   new_timer     (ITimerListener*)        = 0;
    virtual BackendPrepare* new_prepare   (IPrepareListener*)      = 0;
    virtual BackendCheck*   new_check     (ICheckListener*)        = 0;
    virtual BackendIdle*    new_idle      (IIdleListener*)         = 0;
    virtual BackendAsync*   new_async     (IAsyncListener*)        = 0;
    virtual BackendSignal*  new_signal    (ISignalListener*)       = 0;
    virtual BackendPoll*    new_poll_sock (IPollListener*, sock_t) = 0;
    virtual BackendPoll*    new_poll_fd   (IPollListener*, int)    = 0;
    virtual BackendTick*    new_tick      (ITickListener*)         = 0;

    virtual ~BackendLoop () {}

private:
    Loop* _frontend;
};

}}}
