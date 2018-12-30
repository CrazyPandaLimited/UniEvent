#pragma once
#include "Handle.h"
#include "forward.h"
#include "backend/BackendPrepare.h"

namespace panda { namespace unievent {

struct Prepare : virtual Handle {
    using prepare_fptr = void(Prepare* handle);
    using prepare_fn = function<prepare_fptr>;

    CallbackDispatcher<prepare_fptr> prepare_event;

    Prepare (Loop* loop = Loop::default_loop()) {
        _init(loop->impl()->new_prepare(this));
    }

    const HandleType& type () const override;

    virtual void start (prepare_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_on_prepare () { on_prepare(); }

    static PrepareSP call_soon (function<void()> f, Loop* loop = Loop::default_loop());

    static const HandleType Type;

protected:
    virtual void on_prepare ();

    backend::BackendPrepare* impl () const { return static_cast<backend::BackendPrepare*>(_impl); }
};

}}
