#pragma once
#include "Handle.h"
#include "backend/BackendIdle.h"

namespace panda { namespace unievent {

struct Idle : virtual BHandle, private backend::IIdleListener {
    using idle_fptr = void(const IdleSP&);
    using idle_fn   = function<idle_fptr>;
    
    CallbackDispatcher<idle_fptr> event;

    Idle (const LoopSP& loop = Loop::default_loop()) {
        _init(loop, loop->impl()->new_idle(this));
    }

    const HandleType& type () const override;

    virtual void start (idle_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;
    void clear () override;

    void call_now () { on_idle(); }

    static const HandleType TYPE;

protected:
    virtual void on_idle ();

private:
    void handle_idle () override;

    backend::BackendIdle* impl () const { return static_cast<backend::BackendIdle*>(_impl); }
};

}}
