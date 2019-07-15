#include "Signal.h"
using namespace panda::unievent;

panda::string Signal::_signames[NSIG];
bool Signal::__init_ = __init();

bool Signal::__init () {
    _signames[SIGHUP]    = "SIGHUP";
    _signames[SIGINT]    = "SIGINT";
    _signames[SIGQUIT]   = "SIGQUIT";
    _signames[SIGILL]    = "SIGILL";
    _signames[SIGTRAP]   = "SIGTRAP";
    _signames[SIGABRT]   = "SIGABRT";
    _signames[SIGBUS]    = "SIGBUS";
    _signames[SIGFPE]    = "SIGFPE";
    _signames[SIGKILL]   = "SIGKILL";
    _signames[SIGUSR1]   = "SIGUSR1";
    _signames[SIGSEGV]   = "SIGSEGV";
    _signames[SIGUSR2]   = "SIGUSR2";
    _signames[SIGPIPE]   = "SIGPIPE";
    _signames[SIGALRM]   = "SIGALRM";
    _signames[SIGTERM]   = "SIGTERM";
#if defined(SIGSTKFLT)
    _signames[SIGSTKFLT] = "SIGSTKFLT";
#endif
    _signames[SIGCHLD]   = "SIGCHLD";
    _signames[SIGCONT]   = "SIGCONT";
    _signames[SIGSTOP]   = "SIGSTOP";
    _signames[SIGTSTP]   = "SIGTSTP";
    _signames[SIGTTIN]   = "SIGTTIN";
    _signames[SIGTTOU]   = "SIGTTOU";
    _signames[SIGURG]    = "SIGURG";
    _signames[SIGXCPU]   = "SIGXCPU";
    _signames[SIGXFSZ]   = "SIGXFSZ";
    _signames[SIGVTALRM] = "SIGVTALRM";
    _signames[SIGPROF]   = "SIGPROF";
    _signames[SIGWINCH]  = "SIGWINCH";
#if defined(SIGIO)
    _signames[SIGIO]     = "SIGIO";
#endif
#if defined(SIGPOLL)
    _signames[SIGPOLL]   = "SIGPOLL";
#endif
#if defined(SIGPWR)
    _signames[SIGPWR]    = "SIGPWR";
#endif
    _signames[SIGSYS]    = "SIGSYS";
    return true;
}

void Signal::uvx_on_signal (uv_signal_t* handle, int signum) {
    Signal* h = hcast<Signal*>(handle);
    h->call_on_signal(signum);
}

void Signal::start (int signum, signal_fn callback) {
    if (callback) signal_event.add(callback);
    int err = uv_signal_start(&uvh, uvx_on_signal, signum);
    if (err) throw CodeError(err);
}

void Signal::start_once (int signum, signal_fn callback) {
    if (callback) signal_event.add(callback);
    int err = uv_signal_start_oneshot(&uvh, uvx_on_signal, signum);
    if (err) throw CodeError(err);
}

void Signal::stop () {
    int err = uv_signal_stop(&uvh);
    if (err) throw CodeError(err);
}

void Signal::reset () { stop(); }

void Signal::on_signal (int signum) {
    if (signal_event.has_listeners()) signal_event(this, signum);
    else throw ImplRequiredError("Signal::on_signal");
}
