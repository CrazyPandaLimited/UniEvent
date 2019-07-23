#pragma once
#include "BackendHandle.h"
#include "backend/AsyncImpl.h"

namespace panda { namespace unievent {

struct Async : virtual BackendHandle, private backend::IAsyncListener {
    using async_fptr = void(const AsyncSP&);
    using async_fn   = function<async_fptr>;
    
    CallbackDispatcher<async_fptr> event;

    Async (const LoopSP& loop = Loop::default_loop()) {
        _init(loop, loop->impl()->new_async(this));
    }

    Async (async_fn cb, Loop* loop = Loop::default_loop()) : Async(loop) {
        if (cb) event.add(cb);
    }

    const HandleType& type () const override;

    virtual void send ();

    void reset () override {}
    void clear () override { weak(false); }

    void call_now () { on_async(); }

    static const HandleType TYPE;

protected:
    virtual void on_async ();

private:
    void handle_async () override;

    backend::AsyncImpl* impl () const { return static_cast<backend::AsyncImpl*>(_impl); }
};

}}
