#pragma once
#include "Handle.h"

namespace panda { namespace unievent {

struct Idle : virtual Handle {
    using idle_fptr = void(Idle*);
    using idle_fn = function<idle_fptr>;
    
    CallbackDispatcher<idle_fptr> idle_event;

    Idle (Loop* loop = Loop::default_loop()) {
        uv_idle_init(_pex_(loop), &uvh);
        _init(&uvh);
    }

    virtual void start (idle_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_on_idle () { on_idle(); }

protected:
    virtual void on_idle ();

private:
    uv_idle_t uvh;

    static void uvx_on_idle (uv_idle_t* handle);
};

}}
