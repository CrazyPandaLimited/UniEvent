#include "XSHandles.h"

namespace xs { namespace unievent {

static thread_local Stash _perl_handle_classes[HTYPE_MAX+1];

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
static const auto evname_on_send	          = Simple::shared("on_send");
static const auto evname_on_create_connection = Simple::shared("on_create_connection");

Ref stat2avr (const stat_t* stat) {
    return Ref::create(Array::create({
        Simple(stat->st_dev),
        Simple(stat->st_ino),
        Simple(stat->st_mode),
        Simple(stat->st_nlink),
        Simple(stat->st_uid),
        Simple(stat->st_gid),
        Simple(stat->st_rdev),
        Simple(stat->st_size),
        Simple(stat->st_atim.tv_sec),
        Simple(stat->st_mtim.tv_sec),
        Simple(stat->st_ctim.tv_sec),
        Simple(stat->st_blksize),
        Simple(stat->st_blocks),
        Simple(stat->st_flags),
        Simple(stat->st_gen),
        Simple(stat->st_birthtim.tv_sec),
    }));
}

Ref stat2hvr (const stat_t* stat) {
    return Ref::create(Hash::create({
        {"dev",       Simple(stat->st_dev)},
        {"ino",       Simple(stat->st_ino)},
        {"mode",      Simple(stat->st_mode)},
        {"nlink",     Simple(stat->st_nlink)},
        {"uid",       Simple(stat->st_uid)},
        {"gid",       Simple(stat->st_gid)},
        {"rdev",      Simple(stat->st_rdev)},
        {"size",      Simple(stat->st_size)},
        {"atime",     Simple(stat->st_atim.tv_sec)},
        {"mtime",     Simple(stat->st_mtim.tv_sec)},
        {"ctime",     Simple(stat->st_ctim.tv_sec)},
        {"blksize",   Simple(stat->st_blksize)},
        {"blocks",    Simple(stat->st_blocks)},
        {"flags",     Simple(stat->st_flags)},
        {"gen",       Simple(stat->st_gen)},
        {"birthtime", Simple(stat->st_birthtim.tv_sec)},
    }));
}

void XSPrepare::on_prepare () {
    auto obj = xs::out<Prepare*>(aTHX_ this);
    if (!prepare_xscb.call(obj, evname_on_prepare)) Prepare::on_prepare();
}

void XSCheck::on_check () {
    auto obj = xs::out<Check*>(aTHX_ this);
    if (!check_xscb.call(obj, evname_on_check)) Check::on_check();
}

void XSIdle::on_idle () {
    auto obj = xs::out<Idle*>(aTHX_ this);
    if (!idle_xscb.call(obj, evname_on_idle)) Idle::on_idle();
}

void XSTimer::on_timer () {
    auto obj = xs::out<Timer*>(aTHX_ this);
    if (!timer_xscb.call(obj, evname_on_timer)) Timer::on_timer();
}

void XSFSEvent::on_fs_event (const char* filename, int events) {
    auto obj = xs::out<FSEvent*>(aTHX_ this);
    if (!fs_event_xscb.call(obj, evname_on_fs_event, {
        Simple(filename),
        Simple(events)
    })) FSEvent::on_fs_event(filename, events);
}

void XSFSPoll::on_fs_poll (const stat_t* prev, const stat_t* curr, const CodeError* err) {
    auto obj = xs::out<FSPoll*>(aTHX_ this);
    if (!fs_poll_xscb.call(obj, evname_on_fs_poll, {
        err ? Scalar::undef : (stat_as_hash ? stat2hvr(prev) : stat2avr(prev)),
        err ? Scalar::undef : (stat_as_hash ? stat2hvr(curr) : stat2avr(curr)),
        xs::out(err)
    })) FSPoll::on_fs_poll(prev, curr, err);
}

void XSSignal::on_signal (int signum) {
    _EDEBUGTHIS();
    auto obj = xs::out<Signal*>(aTHX_ this);
    if (!signal_xscb.call(obj, evname_on_signal, { Simple(signum) })) Signal::on_signal(signum);
}

void XSStream::on_connection (StreamSP stream, const CodeError* err) {
    _EDEBUGTHIS();
    auto obj = xs::out<Stream*>(aTHX_ this);
    if (!connection_xscb.call(obj, evname_on_connection, { xs::out(stream.get()), xs::out(err) })) Stream::on_connection(stream, err);
}

void XSStream::on_connect (const CodeError* err, ConnectRequest* req) {
    _EDEBUGTHIS();
    auto obj = xs::out<Stream*>(aTHX_ this);
    if (!connect_xscb.call(obj, evname_on_connect, { xs::out(err) })) Stream::on_connect(err, req);
}

void XSStream::on_read (string& buf, const CodeError* err) {
    _EDEBUGTHIS();
    auto obj = xs::out<Stream*>(aTHX_ this);
    if (!read_xscb.call(obj, evname_on_read, {
        err ? Scalar::undef : Simple(string_view(buf.data(), buf.length())),
        xs::out(err)
    })) Stream::on_read(buf, err);
}

void XSStream::on_write (const CodeError* err, WriteRequest* req) {
    _EDEBUGTHIS();
    auto obj = xs::out<Stream*>(aTHX_ this);
    if (!write_xscb.call(obj, evname_on_write, { xs::out(err) })) Stream::on_write(err, req);
}

void XSStream::on_shutdown (const CodeError* err, ShutdownRequest* req) {
    _EDEBUGTHIS();
    auto obj = xs::out<Stream*>(aTHX_ this);
    if (!shutdown_xscb.call(obj, evname_on_shutdown, { xs::out(err) })) Stream::on_shutdown(err, req);
}

void XSStream::on_eof () {
    _EDEBUGTHIS();
    auto obj = xs::out<Stream*>(aTHX_ this);
    if (!eof_xscb.call(obj, evname_on_eof)) Stream::on_eof();
}

StreamSP XSTCP::on_create_connection () {
    TCPSP ret = make_backref<XSTCP>(loop());
    xs::out<TCP*>(ret.get());
    return ret;
}

void XSUDP::on_receive (string& buf, const SockAddr& sa, unsigned flags, const CodeError* err) {
    auto obj = xs::out<UDP*>(aTHX_ this);
    if (!receive_xscb.call(obj, evname_on_receive, {
        err ? Scalar::undef : Simple(string_view(buf.data(), buf.length())),
        xs::out(&sa),
        Simple(flags),
        xs::out(err)
    })) UDP::on_receive(buf, sa, flags, err);
}

void XSUDP::on_send (const CodeError* err, SendRequest* req) {
    auto obj = xs::out<UDP*>(aTHX_ this);
    if (!send_xscb.call(obj, evname_on_send, { xs::out(err) })) UDP::on_send(err, req);
}

StreamSP XSPipe::on_create_connection () {
    PipeSP ret = make_backref<XSPipe>(ipc, loop());
    xs::out<Pipe*>(ret.get());
    return ret;
}

StreamSP XSTTY::on_create_connection () {
    TTYSP ret = make_backref<XSTTY>(fd, readable(), loop());
    xs::out<TTY*>(ret.get());
    return ret;
}

Stash perl_class_for_handle (Handle* h) {
    auto ca = _perl_handle_classes;
    if (!ca[HTYPE_TCP]) {
        ca[HTYPE_CHECK]      = Stash("UniEvent::Check",   GV_ADD);
        ca[HTYPE_FS_EVENT]   = Stash("UniEvent::FSEvent", GV_ADD);
        ca[HTYPE_FS_POLL]    = Stash("UniEvent::FSPoll",  GV_ADD);
        ca[HTYPE_IDLE]       = Stash("UniEvent::Idle",    GV_ADD);
        ca[HTYPE_NAMED_PIPE] = Stash("UniEvent::Pipe",    GV_ADD);
        ca[HTYPE_POLL]       = Stash("UniEvent::Poll",    GV_ADD);
        ca[HTYPE_PREPARE]    = Stash("UniEvent::Prepare", GV_ADD);
        ca[HTYPE_PROCESS]    = Stash("UniEvent::Process", GV_ADD);
        ca[HTYPE_TCP]        = Stash("UniEvent::TCP",     GV_ADD);
        ca[HTYPE_TIMER]      = Stash("UniEvent::Timer",   GV_ADD);
        ca[HTYPE_TTY]        = Stash("UniEvent::TTY",     GV_ADD);
        ca[HTYPE_UDP]        = Stash("UniEvent::UDP",     GV_ADD);
        ca[HTYPE_SIGNAL]     = Stash("UniEvent::Signal",  GV_ADD);
        ca[HTYPE_FILE]       = Stash("UniEvent::File",    GV_ADD);
    }
    assert(ca[h->type()]);
    return ca[h->type()];
}

}}
