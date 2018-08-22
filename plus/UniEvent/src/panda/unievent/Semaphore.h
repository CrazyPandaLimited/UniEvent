#pragma once
#include <panda/unievent/Error.h>

namespace panda { namespace unievent {

class Semaphore {
public:
    Semaphore (unsigned int value) {
        int err = uv_sem_init(&handle, value);
        if (err) throw ThreadError(err);
    }

    virtual void post    ();
    virtual void wait    ();
    virtual int  trywait ();

    virtual ~Semaphore ();
private:
    uv_sem_t handle;
};

}}
