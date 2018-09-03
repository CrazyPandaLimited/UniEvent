#pragma once
#include <panda/unievent/Handle.h>

namespace panda { namespace unievent {

class Async : public virtual Handle {
public:
    typedef function<void(Async* handle)> async_fn;
    
    async_fn async_callback;

    Async (async_fn callback, Loop* loop = Loop::default_loop()) {
        async_callback =  callback;
        _init(&uvh);
        int err = uv_async_init(_pex_(loop), &uvh, uvx_on_async);
        if (err) throw CodeError(err);
    }

    virtual void send ();

    void reset () override;

    void call_on_async () { on_async(); }

protected:
    virtual void on_async ();

private:
    uv_async_t uvh;

    static void uvx_on_async (uv_async_t* handle);
};

}}
