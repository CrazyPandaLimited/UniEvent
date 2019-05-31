#pragma once
#include "Handle.h"
#include "backend/BackendAsync.h"

namespace panda { namespace unievent {

struct Async : virtual BHandle, private backend::IAsyncListener {
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
    void clear () override {}

    void call_now () { on_async(); }

    static const HandleType TYPE;

protected:
    virtual void on_async ();

private:
    void handle_async () override;

    backend::BackendAsync* impl () const { return static_cast<backend::BackendAsync*>(_impl); }
};

}}
