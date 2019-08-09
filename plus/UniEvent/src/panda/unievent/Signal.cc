#include "Signal.h"
using namespace panda::unievent;

static panda::string signames[NSIG];

static bool init () {
    signames[SIGHUP]    = "SIGHUP";
    signames[SIGINT]    = "SIGINT";
    signames[SIGQUIT]   = "SIGQUIT";
    signames[SIGILL]    = "SIGILL";
    signames[SIGTRAP]   = "SIGTRAP";
    signames[SIGABRT]   = "SIGABRT";
    signames[SIGBUS]    = "SIGBUS";
    signames[SIGFPE]    = "SIGFPE";
    signames[SIGKILL]   = "SIGKILL";
    signames[SIGUSR1]   = "SIGUSR1";
    signames[SIGSEGV]   = "SIGSEGV";
    signames[SIGUSR2]   = "SIGUSR2";
    signames[SIGPIPE]   = "SIGPIPE";
    signames[SIGALRM]   = "SIGALRM";
    signames[SIGTERM]   = "SIGTERM";
#if defined(SIGSTKFLT)
    signames[SIGSTKFLT] = "SIGSTKFLT";
#endif
    signames[SIGCHLD]   = "SIGCHLD";
    signames[SIGCONT]   = "SIGCONT";
    signames[SIGSTOP]   = "SIGSTOP";
    signames[SIGTSTP]   = "SIGTSTP";
    signames[SIGTTIN]   = "SIGTTIN";
    signames[SIGTTOU]   = "SIGTTOU";
    signames[SIGURG]    = "SIGURG";
    signames[SIGXCPU]   = "SIGXCPU";
    signames[SIGXFSZ]   = "SIGXFSZ";
    signames[SIGVTALRM] = "SIGVTALRM";
    signames[SIGPROF]   = "SIGPROF";
    signames[SIGWINCH]  = "SIGWINCH";
#if defined(SIGIO)
    signames[SIGIO]     = "SIGIO";
#endif
#if defined(SIGPOLL)
    signames[SIGPOLL]   = "SIGPOLL";
#endif
#if defined(SIGPWR)
    signames[SIGPWR]    = "SIGPWR";
#endif
    signames[SIGSYS]    = "SIGSYS";
    return true;
}
static bool _init = init();

const HandleType Signal::TYPE("signal");

const HandleType& Signal::type () const {
    return TYPE;
}

void Signal::start (int signum, signal_fn callback) {
    if (callback) event.add(callback);
    impl()->start(signum);
}

void Signal::once (int signum, signal_fn callback) {
    if (callback) event.add(callback);
    impl()->once(signum);
}

SignalSP Signal::watch (int signum, signal_fn callback, const LoopSP& loop) {
    SignalSP h = new Signal(loop);
    h->start(signum, callback);
    return h;
}

void Signal::stop () {
    impl()->stop();
}

void Signal::reset () {
    stop();
}

void Signal::clear () {
    stop();
    weak(false);
    _listener = nullptr;
    event.remove_all();
}

void Signal::handle_signal (int signum) {
    SignalSP self = this;
    event(self, signum);
    if (_listener) _listener->on_signal(self, signum);
}

const panda::string& Signal::signame (int signum) {
    if (signum < 0 || signum >= NSIG) throw std::invalid_argument("signum must be >= 0 and < NSIG");
    return signames[signum];
}
