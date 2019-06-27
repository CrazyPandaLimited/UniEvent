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

Stash xs::unievent::perl_class_for_handle (Handle* h) {
    auto& ca = _perl_handle_classes;
    if (!ca.size()) {
        ca[Prepare::TYPE] = Stash("UniEvent::Prepare", GV_ADD);
        ca[Check::TYPE]   = Stash("UniEvent::Check",   GV_ADD);
        ca[Idle::TYPE]    = Stash("UniEvent::Idle",    GV_ADD);
        ca[Timer::TYPE]   = Stash("UniEvent::Timer",   GV_ADD);
        ca[Signal::TYPE]  = Stash("UniEvent::Signal",  GV_ADD);
        ca[Poll::TYPE]    = Stash("UniEvent::Poll",    GV_ADD);
        ca[Udp::TYPE]     = Stash("UniEvent::Udp",     GV_ADD);
        ca[Pipe::TYPE]    = Stash("UniEvent::Pipe",    GV_ADD);
        ca[Tcp::TYPE]     = Stash("UniEvent::Tcp",     GV_ADD);
        ca[Tty::TYPE]     = Stash("UniEvent::Tty",     GV_ADD);
        ca[FsPoll::TYPE]  = Stash("UniEvent::FsPoll",  GV_ADD);
        ca[FsEvent::TYPE] = Stash("UniEvent::FsEvent", GV_ADD);
//        ca[Process::TYPE] = Stash("UniEvent::Process", GV_ADD);
//        ca[File::TYPE]    = Stash("UniEvent::File",    GV_ADD);
    }
    return ca.at(h->type());
}

string xs::unievent::sv2buf (const Sv& sv) {
    string buf;
    if (sv.is_array_ref()) { // [$str1, $str2, ...]
        Array wlist(sv);
        STRLEN sum = 0;
        for (auto it = wlist.cbegin(); it != wlist.cend(); ++it) {
            STRLEN len;
            SvPV(*it, len);
            sum += len;
        }
        if (!sum) return string();

        char* ptr = buf.reserve(sum);
        for (auto it = wlist.cbegin(); it != wlist.cend(); ++it) {
            STRLEN len;
            const char* data = SvPV(*it, len);
            memcpy(ptr, data, len);
            ptr += len;
        }
        buf.length(sum);
    } else { // $str
        STRLEN len;
        const char* data = SvPV(sv, len);
        if (!len) return string();
        buf.assign(data, len);
    }
    return buf;
}

static inline void xscall (const Object& handle, const Simple& evname, std::initializer_list<Scalar> args = {}) {
    if (!handle) return; // object is being destroyed
    Sub cv = handle.method(evname); // evname is recommended to be a shared hash string for performance.
    if (cv) cv.call(handle.ref(), args);
}

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

Sv Typemap<Fs::Stat>::out (pTHX_ const Fs::Stat& s, const Sv&) {
    return Ref::create(Array::create({
        Simple(s.dev),
        Simple(s.ino),
        Simple(s.mode),
        Simple(s.nlink),
        Simple(s.uid),
        Simple(s.gid),
        Simple(s.rdev),
        Simple(s.size),
        Simple(s.atime.get()),
        Simple(s.mtime.get()),
        Simple(s.ctime.get()),
        Simple(s.blksize),
        Simple(s.blocks),
        Simple(s.flags),
        Simple(s.gen),
        Simple(s.birthtime.get()),
        Simple((int)s.type()),
        Simple(s.perms()),
    }));
}

Fs::Stat Typemap<Fs::Stat>::in (pTHX_ const Array& a) {
    Fs::Stat ret;
    ret.dev       = Simple(a[0]);
    ret.ino       = Simple(a[1]);
    ret.mode      = Simple(a[2]);
    ret.nlink     = Simple(a[3]);
    ret.uid       = Simple(a[4]);
    ret.gid       = Simple(a[5]);
    ret.rdev      = Simple(a[6]);
    ret.size      = Simple(a[7]);
    ret.atime     = Simple(a[8]);
    ret.mtime     = Simple(a[9]);
    ret.ctime     = Simple(a[10]);
    ret.blksize   = Simple(a[11]);
    ret.blocks    = Simple(a[12]);
    ret.flags     = Simple(a[13]);
    ret.gen       = Simple(a[14]);
    ret.birthtime = Simple(a[15]);
    return ret;
}

Sv Typemap<Fs::DirEntry>::out (pTHX_ const Fs::DirEntry& de, const Sv&) {
    return Ref::create(Array::create({
        Simple(de.name()),
        Simple((int)de.type())
    }));
}

Fs::DirEntry Typemap<Fs::DirEntry>::in (pTHX_ const Array& a) {
    return Fs::DirEntry(Simple(a[0]).as_string(), (Fs::FileType)(int)Simple(a[1]));
}

void XSPrepare::on_prepare () {
    auto obj = xs::out<Prepare*>(this);
    xscall(obj, evname_on_prepare);
    Prepare::on_prepare();
}


void XSCheck::on_check () {
    auto obj = xs::out<Check*>(this);
    xscall(obj, evname_on_check);
    Check::on_check();
}


void XSIdle::on_idle () {
    auto obj = xs::out<Idle*>(this);
    xscall(obj, evname_on_idle);
    Idle::on_idle();
}


void XSTimer::on_timer () {
    auto obj = xs::out<Timer*>(this);
    xscall(obj, evname_on_timer);
    Timer::on_timer();
}


void XSSignal::on_signal (int signum) {
    auto obj = xs::out<Signal*>(this);
    xscall(obj, evname_on_signal, { Simple(signum) });
    Signal::on_signal(signum);
}


void XSUdp::on_receive (string& buf, const panda::net::SockAddr& sa, unsigned flags, const CodeError& err) {
    auto obj = xs::out<Udp*>(this);
    xscall(obj, evname_on_receive, {
        err ? Scalar::undef : Simple(string_view(buf.data(), buf.length())),
        xs::out(&sa),
        Simple(flags),
        xs::out<const CodeError&>(err)
    });
    Udp::on_receive(buf, sa, flags, err);
}

void XSUdp::on_send (const CodeError& err, const SendRequestSP& req) {
    auto obj = xs::out<Udp*>(this);
    xscall(obj, evname_on_send, { xs::out<const CodeError&>(err), xs::out(req) });
    Udp::on_send(err, req);
}


void XSStream::on_connection (const StreamSP& client, const CodeError& err) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    xscall(self, evname_on_connection, { xs::out(client.get()), xs::out<const CodeError&>(err) });
    Stream::on_connection(client, err);
}

void XSStream::on_connect (const CodeError& err, const ConnectRequestSP& req) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    xscall(self, evname_on_connect, { xs::out<const CodeError&>(err), xs::out(req) });
    Stream::on_connect(err, req);
}

void XSStream::on_read (string& buf, const CodeError& err) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    xscall(self, evname_on_read, { err ? Scalar::undef : Scalar(xs::out(buf)), xs::out<const CodeError&>(err) });
    Stream::on_read(buf, err);
}

void XSStream::on_write (const CodeError& err, const WriteRequestSP& req) {
    _EDEBUGTHIS();
    auto obj = xs::out<Stream*>(this);
    xscall(obj, evname_on_write, { xs::out<const CodeError&>(err), xs::out(req) });
    Stream::on_write(err, req);
}

void XSStream::on_eof () {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(this);
    xscall(self, evname_on_eof);
    Stream::on_eof();
}

void XSStream::on_shutdown (const CodeError& err, const ShutdownRequestSP& req) {
    _EDEBUGTHIS();
    auto self = xs::out<Stream*>(aTHX_ this);
    xscall(self, evname_on_shutdown, { xs::out<const CodeError&>(err), xs::out(req) });
    Stream::on_shutdown(err, req);
}


StreamSP XSPipe::create_connection () {
    PipeSP ret = make_backref<XSPipe>(loop(), ipc());
    xs::out(ret.get()); // fill backref
    return ret;
}


StreamSP XSTcp::create_connection () {
    TcpSP ret = make_backref<XSTcp>(loop());
    xs::out(ret.get());
    return ret;
}


StreamSP XSTty::create_connection () {
    TtySP ret = make_backref<XSTty>(fd, loop());
    xs::out(ret.get());
    return ret;
}


void XSFsPoll::on_fs_poll (const Fs::Stat& prev, const Fs::Stat& cur, const CodeError& err) {
    _EDEBUGTHIS();
    auto self = xs::out<FsPoll*>(this);
    xscall(self, evname_on_fs_poll, {
        xs::out(prev),
        xs::out(cur),
        xs::out<const CodeError&>(err)
    });
    FsPoll::on_fs_poll(prev, cur, err);
}


void XSFsEvent::on_fs_event (const std::string_view& file, int events, const CodeError& err) {
    auto self = xs::out<FsEvent*>(this);
    xscall(self, evname_on_fs_event, {
        Simple(file),
        Simple(events),
        xs::out<const CodeError&>(err)
    });
    FsEvent::on_fs_event(file, events, err);
}
