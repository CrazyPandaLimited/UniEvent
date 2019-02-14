#pragma once
#include <panda/refcnt.h>

namespace panda {

struct Refcntd : Refcnt {
    void release () const {
        if (refcnt() <= 1) const_cast<Refcntd*>(this)->on_delete();
        Refcnt::release();
    }

protected:
    virtual void on_delete () {}
};

inline void refcnt_dec (const Refcntd* o) { o->release(); }

}
