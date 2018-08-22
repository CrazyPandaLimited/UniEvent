#pragma once
#include <panda/unievent/Error.h>

namespace panda { namespace unievent {

class RWLock {
public:
    RWLock () {
        int err = uv_rwlock_init(&handle);
        if (err) throw ThreadError(err);
    }

    virtual void rdlock    ();
    virtual void wrlock    ();
    virtual void rdunlock  ();
    virtual void wrunlock  ();
    virtual int  tryrdlock ();
    virtual int  trywrlock ();

    virtual ~RWLock ();

private:
    uv_rwlock_t handle;
};

}}
