#pragma once
#include "Handle.h"

namespace panda { namespace unievent {

struct Prepare;
using PrepareSP = iptr<Prepare>;

struct Prepare : virtual Handle {
    using prepare_fptr = void(Prepare* handle);
    using prepare_fn = function<prepare_fptr>;

    CallbackDispatcher<prepare_fptr> prepare_event;

    Prepare (Loop* loop = Loop::default_loop()) {
        uv_prepare_init(_pex_(loop), &uvh);
        _init(&uvh);
    }

    virtual void start (prepare_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_on_prepare () { on_prepare(); }

    static PrepareSP call_soon(function<void()> f, Loop* loop = Loop::default_loop());

protected:
    virtual void on_prepare ();

private:
    uv_prepare_t uvh;

    static void uvx_on_prepare (uv_prepare_t* handle);
};

}}
