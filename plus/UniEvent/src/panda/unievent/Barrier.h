#pragma once
#include "Error.h"

namespace panda { namespace unievent {

struct Barrier {
    Barrier (unsigned int count) {
        int err = uv_barrier_init(&handle, count);
        if (err) throw CodeError(err);
    }

    virtual int wait ();

    virtual ~Barrier ();

private:
    uv_barrier_t handle;
};

}}
