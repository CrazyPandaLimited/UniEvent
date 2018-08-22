#pragma once
#include <panda/unievent/Error.h>
#include <panda/unievent/Mutex.h>

namespace panda { namespace unievent {

class Condition {
public:
    Condition () {
        int err = uv_cond_init(&handle);
        if (err) throw ThreadError(err);
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
