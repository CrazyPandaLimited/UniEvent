#pragma once
#include <panda/unievent/Error.h>

namespace panda { namespace unievent {

class Mutex {
public:
    Mutex (bool recursive = false) {
        int err = recursive ? uv_mutex_init_recursive(&handle) : uv_mutex_init(&handle);
        if (err) throw ThreadError(err);
    }

    void lock    () { uv_mutex_lock(&handle); }
    void unlock  () { uv_mutex_unlock(&handle); }
    int  trylock () { return uv_mutex_trylock(&handle); }

    virtual ~Mutex () {
        uv_mutex_destroy(&handle);
    }

    friend uv_mutex_t* _pex_ (Mutex*);

private:
    uv_mutex_t handle;
};

inline uv_mutex_t* _pex_ (Mutex* mutex) { return &mutex->handle; }

}}
