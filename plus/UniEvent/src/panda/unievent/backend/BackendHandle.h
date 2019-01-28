#pragma once
#include "BackendLoop.h"

namespace panda { namespace unievent { namespace backend {

struct BackendHandle {
    virtual BackendLoop* loop () const = 0;

    virtual bool active () const = 0;

    virtual void set_weak   () = 0;
    virtual void unset_weak () = 0;

    virtual void destroy () = 0;

    virtual ~BackendHandle () {}
};

}}}
