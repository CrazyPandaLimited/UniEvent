#pragma once
#include "Handle.h"
#include "backend/BackendCheck.h"

namespace panda { namespace unievent {

struct Check : virtual BHandle, private backend::ICheckListener {
    using check_fptr = void(const CheckSP&);
    using check_fn   = function<check_fptr>;

    CallbackDispatcher<check_fptr> event;

    Check (const LoopSP& loop = Loop::default_loop()) {
        _init(loop, loop->impl()->new_check(this));
    }

    const HandleType& type () const override;

    virtual void start (check_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;
    void clear () override;

    void call_now () { on_check(); }

    static const HandleType TYPE;

protected:
    virtual void on_check ();

private:
    void handle_check () override;

    backend::BackendCheck* impl () const { return static_cast<backend::BackendCheck*>(_impl); }
};

}}
