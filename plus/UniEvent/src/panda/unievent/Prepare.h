#pragma once
#include "Handle.h"
#include "backend/BackendPrepare.h"

namespace panda { namespace unievent {

struct Prepare : virtual Handle {
    using prepare_fptr = void(Prepare*);
    using prepare_fn   = function<prepare_fptr>;

    CallbackDispatcher<prepare_fptr> prepare_event;

    Prepare (Loop* loop = Loop::default_loop()) {
        _init(loop_impl(loop)->new_prepare(this));
    }

    ~Prepare () { _EDTOR(); }

    const HandleType& type () const override;

    virtual void start (prepare_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_now () { on_prepare(); }

    static const HandleType TYPE;

protected:
    virtual void on_prepare ();

    backend::BackendPrepare* impl () const { return static_cast<backend::BackendPrepare*>(Handle::impl()); }
};

}}
