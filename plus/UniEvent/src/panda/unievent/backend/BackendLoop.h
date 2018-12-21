#pragma once
#include "forward.h"

namespace panda { namespace unievent { namespace backend {

struct BackendLoop {
    enum class Type { LOCAL, GLOBAL, DEFAULT };

    BackendLoop (Loop* frontend) : _frontend(frontend) {}

    Loop* frontend () const { return _frontend; }

    virtual int  run         () = 0;
    virtual int  run_once    () = 0;
    virtual int  run_nowait  () = 0;
    virtual void stop        () = 0;
    virtual void handle_fork () = 0;

    virtual BackendTimer* new_timer (Timer* frontend) = 0;

    virtual ~BackendLoop () {}

private:
    Loop* _frontend;
};

}}}
