#pragma once
#include "Handle.h"
#include <signal.h>
#include "backend/BackendSignal.h"

namespace panda { namespace unievent {

struct Signal : virtual Handle, private backend::ISignalListener {
    using signal_fptr = void(const SignalSP& handle, int signum);
    using signal_fn = function<signal_fptr>;
    
    CallbackDispatcher<signal_fptr> event;

    Signal (const LoopSP& loop = Loop::default_loop()) {
        _init(loop, loop->impl()->new_signal(this));
    }

    const HandleType& type () const override;

    int           signum  () const { return impl()->signum(); }
    const string& signame () const { return signame(signum()); }

    virtual void start (int signum, signal_fn callback = {});
    virtual void once  (int signum, signal_fn callback = {});
    virtual void stop  ();

    void reset () override;

    void call_now (int signum) { on_signal(signum); }

    static const HandleType TYPE;

    static const string& signame (int signum);

protected:
    virtual void on_signal (int signum);

private:
    void handle_signal (int signum) override;

    backend::BackendSignal* impl () const { return static_cast<backend::BackendSignal*>(_impl); }
};

}}
