#include "unievent.h"
#include <typeinfo>
#include <cxxabi.h>

using xs::my_perl;
using namespace xs;
using namespace xs::unievent;
using namespace panda::unievent;

static const std::string cpp_errns  = "panda::unievent::";
static const std::string perl_errns = "UniEvent::";

static thread_local std::map<HandleType, Stash> _perl_handle_classes;

static const auto evname_on_prepare           = Simple::shared("on_prepare");
static const auto evname_on_check             = Simple::shared("on_check");
static const auto evname_on_idle              = Simple::shared("on_idle");
static const auto evname_on_timer             = Simple::shared("on_timer");
static const auto evname_on_fs_event          = Simple::shared("on_fs_event");
static const auto evname_on_fs_poll           = Simple::shared("on_fs_poll");
static const auto evname_on_signal            = Simple::shared("on_signal");
static const auto evname_on_connection        = Simple::shared("on_connection");
static const auto evname_on_read              = Simple::shared("on_read");
static const auto evname_on_write             = Simple::shared("on_write");
static const auto evname_on_shutdown          = Simple::shared("on_shutdown");
static const auto evname_on_eof               = Simple::shared("on_eof");
static const auto evname_on_connect           = Simple::shared("on_connect");
static const auto evname_on_receive           = Simple::shared("on_receive");
static const auto evname_on_send              = Simple::shared("on_send");
static const auto evname_on_create_connection = Simple::shared("on_create_connection");

Stash xs::unievent::get_perl_class_for_err (const Error& err) {
    int status;
    char* cpp_class_name = abi::__cxa_demangle(typeid(err).name(), nullptr, nullptr, &status);
    if (status != 0) throw "[UniEvent] !critical! abi::__cxa_demangle error";
    if (strstr(cpp_class_name, cpp_errns.c_str()) != cpp_class_name) throw "[UniEvent] strange exception caught";
    std::string stash_name(perl_errns);
    stash_name.append(cpp_class_name + cpp_errns.length());
    free(cpp_class_name);
    Stash stash(std::string_view(stash_name.data(), stash_name.length()));
    if (!stash) throw Simple::format("[UniEvent] !critical! no error package: %s", stash_name.c_str());
    return stash;
}

Stash perl_class_for_handle (Handle* h) {
    auto& ca = _perl_handle_classes;
    if (!ca.size()) {
        ca[Prepare::TYPE] = Stash("UniEvent::Prepare", GV_ADD);
        ca[Check::TYPE]   = Stash("UniEvent::Check",   GV_ADD);
        ca[Idle::TYPE]    = Stash("UniEvent::Idle",    GV_ADD);
        ca[Timer::TYPE]   = Stash("UniEvent::Timer",   GV_ADD);
        ca[Signal::TYPE]  = Stash("UniEvent::Signal",  GV_ADD);
        ca[Poll::TYPE]    = Stash("UniEvent::Poll",    GV_ADD);
        ca[Udp::TYPE]     = Stash("UniEvent::UDP",     GV_ADD);
        ca[Pipe::TYPE]    = Stash("UniEvent::Pipe",    GV_ADD);
//        ca[FSEvent::Type] = Stash("UniEvent::FSEvent", GV_ADD);
//        ca[FSPoll::Type]  = Stash("UniEvent::FSPoll",  GV_ADD);
//        ca[Process::Type] = Stash("UniEvent::Process", GV_ADD);
//        ca[TCP::Type]     = Stash("UniEvent::TCP",     GV_ADD);
//        ca[TTY::Type]     = Stash("UniEvent::TTY",     GV_ADD);
//        ca[File::Type]    = Stash("UniEvent::File",    GV_ADD);
    }
    return ca.at(h->type());
}

void XSCallback::set (SV* cbsv) {
    if (!cbsv || !SvOK(cbsv)) {
        SvREFCNT_dec(callback);
        callback = nullptr;
        return;
    }

    CV* cv;
    if (!SvROK(cbsv) || SvTYPE(cv = (CV*)SvRV(cbsv)) != SVt_PVCV) croak("[UniEvent] unsupported argument type for setting callback");

    SvREFCNT_dec(callback);
    callback = cv;
    SvREFCNT_inc_simple_void_NN(callback);
}

SV* XSCallback::get () {
    return callback ? newRV((SV*)callback) : SvREFCNT_inc_simple_NN(&PL_sv_undef);
}

bool XSCallback::call (const Object& handle, const Simple& evname, std::initializer_list<Scalar> args) {
    if (!handle) return true; // object is being destroyed

    Sub cv;
    if (evname) cv = handle.method(evname); // default behaviour - call method evname on handle. evname is recommended to be a shared hash string for performance.
    if (!cv) cv = callback;
    if (!cv) return false;

    cv.call(handle.ref(), args);

    return true;
}

//Ref stat2avr (const stat_t* stat) {
//    return Ref::create(Array::create({
//        Simple(stat->st_dev),
//        Simple(stat->st_ino),
//        Simple(stat->st_mode),
//        Simple(stat->st_nlink),
//        Simple(stat->st_uid),
//        Simple(stat->st_gid),
//        Simple(stat->st_rdev),
//        Simple(stat->st_size),
//        Simple(stat->st_atim.tv_sec),
//        Simple(stat->st_mtim.tv_sec),
//        Simple(stat->st_ctim.tv_sec),
//        Simple(stat->st_blksize),
//        Simple(stat->st_blocks),
//        Simple(stat->st_flags),
//        Simple(stat->st_gen),
//        Simple(stat->st_birthtim.tv_sec),
//    }));
//}
//
//Ref stat2hvr (const stat_t* stat) {
//    return Ref::create(Hash::create({
//        {"dev",       Simple(stat->st_dev)},
//        {"ino",       Simple(stat->st_ino)},
//        {"mode",      Simple(stat->st_mode)},
//        {"nlink",     Simple(stat->st_nlink)},
//        {"uid",       Simple(stat->st_uid)},
//        {"gid",       Simple(stat->st_gid)},
//        {"rdev",      Simple(stat->st_rdev)},
//        {"size",      Simple(stat->st_size)},
//        {"atime",     Simple(stat->st_atim.tv_sec)},
//        {"mtime",     Simple(stat->st_mtim.tv_sec)},
//        {"ctime",     Simple(stat->st_ctim.tv_sec)},
//        {"blksize",   Simple(stat->st_blksize)},
//        {"blocks",    Simple(stat->st_blocks)},
//        {"flags",     Simple(stat->st_flags)},
//        {"gen",       Simple(stat->st_gen)},
//        {"birthtime", Simple(stat->st_birthtim.tv_sec)},
//    }));
//}

//void XSCommandCallback::run () {
//    xscb.call(SvRV(handle_rv), nullptr);
//}
//
//void XSCommandCallback::cancel () {}
//
//XSCommandCallback::~XSCommandCallback () {
//    SvREFCNT_dec_NN(handle_rv);
//}

void XSPrepare::on_prepare () {
    auto obj = xs::out<Prepare*>(this);
    prepare_xscb.call(obj, evname_on_prepare);
    Prepare::on_prepare();
}


void XSCheck::on_check () {
    auto obj = xs::out<Check*>(this);
    check_xscb.call(obj, evname_on_check);
    Check::on_check();
}


void XSIdle::on_idle () {
    auto obj = xs::out<Idle*>(this);
    idle_xscb.call(obj, evname_on_idle);
    Idle::on_idle();
}


void XSTimer::on_timer () {
    auto obj = xs::out<Timer*>(this);
    timer_xscb.call(obj, evname_on_timer);
    Timer::on_timer();
}


void XSSignal::on_signal (int signum) {
    auto obj = xs::out<Signal*>(this);
    signal_xscb.call(obj, evname_on_signal, { Simple(signum) });
    Signal::on_signal(signum);
}


void XSUdp::on_receive (string& buf, const panda::net::SockAddr& sa, unsigned flags, const CodeError* err) {
    auto obj = xs::out<Udp*>(this);
    receive_xscb.call(obj, evname_on_receive, {
        err ? Scalar::undef : Simple(string_view(buf.data(), buf.length())),
        xs::out(&sa),
        Simple(flags),
        xs::out(err)
    });
    Udp::on_receive(buf, sa, flags, err);
}

void XSUdp::on_send (const CodeError* err, const SendRequestSP& req) {
    auto obj = xs::out<Udp*>(this);
    send_xscb.call(obj, evname_on_send, { xs::out(err) });
    Udp::on_send(err, req);
}


void XSStream::on_connection (const StreamSP& client, const CodeError* err) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    connection_xscb.call(self, evname_on_connection, { xs::out(client.get()), xs::out(err) });
    Stream::on_connection(client, err);
}

void XSStream::on_connect (const CodeError* err, const ConnectRequestSP& req) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    connect_xscb.call(self, evname_on_connect, { xs::out(err) });
    Stream::on_connect(err, req);
}

void XSStream::on_read (string& buf, const CodeError* err) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    read_xscb.call(self, evname_on_read, {
        err ? Scalar::undef : Simple(string_view(buf.data(), buf.length())),
        xs::out(err)
    });
    Stream::on_read(buf, err);
}

//void XSStream::on_write (const CodeError* err, WriteRequest* req) {
//    _EDEBUGTHIS();
//    auto obj = xs::out<Stream*>(aTHX_ this);
//    if (!write_xscb.call(obj, evname_on_write, { xs::out(err) })) Stream::on_write(err, req);
//}

void XSStream::on_eof () {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    eof_xscb.call(self, evname_on_eof);
    Stream::on_eof();
}

void XSStream::on_shutdown (const CodeError* err, const ShutdownRequestSP& req) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(aTHX_ this);
    shutdown_xscb.call(self, evname_on_shutdown, { xs::out(err) });
    Stream::on_shutdown(err, req);
}


//StreamSP XSPipe::on_create_connection () {
//    PipeSP ret = make_backref<XSPipe>(ipc, loop());
//    xs::out<Pipe*>(ret.get());
//    return ret;
//}

//void XSFSEvent::on_fs_event (const char* filename, int events) {
//    auto obj = xs::out<FSEvent*>(aTHX_ this);
//    if (!fs_event_xscb.call(obj, evname_on_fs_event, {
//        Simple(filename),
//        Simple(events)
//    })) FSEvent::on_fs_event(filename, events);
//}
//
//void XSFSPoll::on_fs_poll (const stat_t* prev, const stat_t* curr, const CodeError* err) {
//    auto obj = xs::out<FSPoll*>(aTHX_ this);
//    if (!fs_poll_xscb.call(obj, evname_on_fs_poll, {
//        err ? Scalar::undef : (stat_as_hash ? stat2hvr(prev) : stat2avr(prev)),
//        err ? Scalar::undef : (stat_as_hash ? stat2hvr(curr) : stat2avr(curr)),
//        xs::out(err)
//    })) FSPoll::on_fs_poll(prev, curr, err);
//}
//
//StreamSP XSTCP::on_create_connection () {
//    TCPSP ret = make_backref<XSTCP>(loop());
//    xs::out<TCP*>(ret.get());
//    return ret;
//}
//
//StreamSP XSTTY::on_create_connection () {
//    TTYSP ret = make_backref<XSTTY>(fd, readable(), loop());
//    xs::out<TTY*>(ret.get());
//    return ret;
//}


static inline void throw_bad_hints () { throw "argument is not a valid AddrInfoHints"; }

AddrInfoHints Typemap<AddrInfoHints>::in (pTHX_ SV* arg) {
    if (!SvOK(arg)) return AddrInfoHints();

    if (SvPOK(arg)) {
        if (SvCUR(arg) < sizeof(AddrInfoHints)) throw_bad_hints();
        return *reinterpret_cast<AddrInfoHints*>(SvPVX(arg));
    }

    if (!Sv(arg).is_hash_ref()) throw_bad_hints();
    AddrInfoHints ret;
    Hash h = arg;
    for (auto& row : h) {
        auto k = row.key();
        auto val = Simple(row.value());
        if      (k == "family"  ) ret.family   = val;
        else if (k == "socktype") ret.socktype = val;
        else if (k == "protocol") ret.protocol = val;
        else if (k == "flags"   ) ret.flags    = val;
    }
    return ret;
}
