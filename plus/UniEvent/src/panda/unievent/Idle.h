#pragma once
#include "Handle.h"
#include "backend/BackendIdle.h"

namespace panda { namespace unievent {

struct Idle : virtual Handle {
    using idle_fptr = void(Idle*);
    using idle_fn   = function<idle_fptr>;
    
    CallbackDispatcher<idle_fptr> idle_event;

    Idle (Loop* loop = Loop::default_loop()) {
        _init(loop->impl()->new_idle(this));
    }

    const HandleType& type () const override;

    virtual void start (idle_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_now () { on_idle(); }

    static const HandleType TYPE;

protected:
    virtual void on_idle ();

    backend::BackendIdle* impl () const { return static_cast<backend::BackendIdle*>(Handle::impl()); }
};

}}
