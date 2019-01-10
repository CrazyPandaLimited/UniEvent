#pragma once
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

    virtual BackendTimer*   new_timer   (Timer*   frontend) = 0;
    virtual BackendPrepare* new_prepare (Prepare* frontend) = 0;

    virtual ~BackendLoop () {}

private:
    Loop* _frontend;
};

}}}
