#pragma once
#include "Handle.h"
#include "backend/BackendCheck.h"

namespace panda { namespace unievent {

struct Check : virtual Handle {
    using check_fptr = void(Check*);
    using check_fn   = function<check_fptr>;

    CallbackDispatcher<check_fptr> check_event;

    Check (Loop* loop = Loop::default_loop()) {
        _init(loop->impl()->new_check(this));
    }

    const HandleType& type () const override;

    virtual void start (check_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_now () { on_check(); }

    static const HandleType TYPE;

protected:
    virtual void on_check ();

    backend::BackendCheck* impl () const { return static_cast<backend::BackendCheck*>(Handle::impl()); }
};

}}
