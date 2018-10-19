#pragma once
#include "Error.h"

namespace panda { namespace unievent {

struct Semaphore {
    Semaphore (unsigned int value) {
        int err = uv_sem_init(&handle, value);
        if (err) throw CodeError(err);
    }

    virtual void post    ();
    virtual void wait    ();
    virtual int  trywait ();

    virtual ~Semaphore ();
private:
    uv_sem_t handle;
};

}}
