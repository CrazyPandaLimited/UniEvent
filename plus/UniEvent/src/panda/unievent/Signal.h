#pragma once
#include <panda/unievent/Handle.h>
#include <signal.h>

namespace panda { namespace unievent {

class Signal : public virtual Handle {
public:
    using signal_fptr = void(Signal* handle, int signum);
    using signal_fn = function<signal_fptr>;
    
    CallbackDispatcher<signal_fptr> signal_event;

    Signal (Loop* loop = Loop::default_loop()) {
        int err = uv_signal_init(_pex_(loop), &uvh);
        if (err) throw SignalError(err);
        _init(&uvh);
    }

    int           signum  () const { return uvh.signum; }
    const string& signame () const { return signame(uvh.signum); }

    virtual void start      (int signum, signal_fn callback = nullptr);
    virtual void start_once (int signum, signal_fn callback = nullptr);
    virtual void stop       ();

    void reset () override;

    static const string& signame (int signum) {
        if (signum < 0 || signum >= NSIG) throw Error("[Signal.signame] signum must be >= 0 and < NSIG");
        return _signames[signum];
    }

    void call_on_signal (int signum) { on_signal(signum); }

protected:
    virtual void on_signal (int signum);

private:
    uv_signal_t uvh;

    static string _signames[NSIG];

    static bool __init_;
    static bool __init ();
    static void uvx_on_signal (uv_signal_t* handle, int signum);
};

}}
