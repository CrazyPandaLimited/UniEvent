#pragma once
#include "Error.h"
#include "Mutex.h"

namespace panda { namespace unievent {

struct Condition {
    Condition () {
        int err = uv_cond_init(&handle);
        if (err) throw CodeError(err);
    }

    virtual void signal    ();
    virtual void broadcast ();
    virtual void wait      (Mutex* mutex);
    virtual int  timedwait (Mutex* mutex, uint64_t timeout);

    virtual ~Condition ();

private:
    uv_cond_t handle;
};

}}
