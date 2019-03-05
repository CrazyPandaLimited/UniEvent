#pragma once
#include "Handle.h"
#include "backend/BackendAsync.h"

namespace panda { namespace unievent {

struct Async : virtual Handle, private backend::IAsyncListener {
    using async_fptr = void(Async*);
    using async_fn   = function<async_fptr>;
    
    CallbackDispatcher<async_fptr> async_event;

    Async (Loop* loop = Loop::default_loop()) {
        _init(loop->impl()->new_async(this));
    }

    Async (async_fn cb, Loop* loop = Loop::default_loop()) : Async(loop) {
        if (cb) async_event.add(cb);
    }

    const HandleType& type () const override;

    virtual void send ();

    void reset () override;

    void call_now () { on_async(); }

    static const HandleType TYPE;

protected:
    void on_async () override;

    backend::BackendAsync* impl () const { return static_cast<backend::BackendAsync*>(Handle::impl()); }
};

}}
