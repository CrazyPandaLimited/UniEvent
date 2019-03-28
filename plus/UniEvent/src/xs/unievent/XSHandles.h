#pragma once
#include "inc.h"
#include "error.h"
#include "XSCallback.h"

#define PEVXS__CB_ACCESSOR(name)                        \
    private:                                            \
    XSCallback _##name;                                 \
    public:                                             \
    void name (SV* callback) { _##name.set(callback); } \
    SV*  name () const       { return _##name.get(); }


namespace xs { namespace unievent {

using namespace panda::unievent;

Stash perl_class_for_handle (Handle* h);

inline file_t sv2file (const Sv& sv) {
    if (sv.is_ref()) return (file_t)PerlIO_fileno(IoIFP( xs::in<IO*>(sv) ));
    else             return (file_t)SvUV(sv);
}

struct XSPrepare : Prepare {
    XSCallback prepare_xscb;
    XSPrepare (Loop* loop) : Prepare(loop) {}
protected:
    void on_prepare () override;
};


struct XSCheck : Check {
    XSCallback check_xscb;
    XSCheck (Loop* loop) : Check(loop) {}
protected:
    void on_check () override;
};


struct XSIdle : Idle {
    XSCallback idle_xscb;
    XSIdle (Loop* loop) : Idle(loop) {}
protected:
    void on_idle () override;
};


struct XSTimer : Timer {
    XSCallback timer_xscb;
    XSTimer (Loop* loop) : Timer(loop) {}
protected:
    void on_timer () override;
};

struct XSSignal : Signal {
    XSCallback signal_xscb;
    XSSignal (Loop* loop) : Signal(loop) {}
protected:
    void on_signal (int signum) override;
};

struct XSUdp : Udp {
    XSCallback receive_xscb;
    XSCallback send_xscb;

    using Udp::Udp;

    void open (const Sv& sv) {
        if (!sv.is_ref()) return open((sock_t)SvUV(sv));
        io_sv = sv;
        Udp::open((sock_t)PerlIO_fileno(IoIFP(xs::in<IO*>(aTHX_ sv))));
    }

    void open (sock_t sock) override {
        io_sv.reset();
        Udp::open(sock);
    }

protected:
    void on_receive (string& buf, const panda::net::SockAddr& sa, unsigned flags, const CodeError* err) override;
    void on_send    (const CodeError* err, const SendRequestSP& req) override;

private:
    Sv io_sv;
};

struct XSStream : virtual Stream {
    XSCallback connection_xscb;
    XSCallback read_xscb;
    XSCallback write_xscb;
    XSCallback shutdown_xscb;
    XSCallback eof_xscb;
    XSCallback connect_xscb;
    XSCallback create_connection_xscb;

    XSStream () {}

protected:
//    void on_connection (StreamSP, const CodeError*) override;
//    void on_connect    (const CodeError*, ConnectRequest*) override;
//    void on_read       (string&, const CodeError*) override;
//    void on_write      (const CodeError*, WriteRequest*) override;
//    void on_shutdown   (const CodeError*, ShutdownRequest*) override;
//    void on_eof        () override;
};

struct XSPipe : Pipe, XSStream {
    XSPipe (Loop* loop, bool ipc) : Pipe(loop, ipc) {}

//    StreamSP on_create_connection () override;
//
//    void open (const Sv& sv) {
//        if (!sv.is_ref()) return open((sock_t)SvUV(sv));
//        io_sv = sv;
//        Pipe::open((file_t)PerlIO_fileno(IoIFP(xs::in<IO*>(aTHX_ sv))));
//    }
//
//    void open (file_t sock) override {
//        io_sv.reset();
//        Pipe::open(sock);
//    }
//
//private:
//    Sv io_sv;
};

//struct XSFSEvent : FSEvent, XSHandle {
//    XSCallback fs_event_xscb;
//    XSFSEvent (Loop* loop) : FSEvent(loop) {}
//protected:
//    void on_fs_event (const char* filename, int events) override;
//};
//
//
//struct XSFSPoll : FSPoll, XSHandle {
//    XSCallback fs_poll_xscb;
//    bool       stat_as_hash;
//    XSFSPoll (Loop* loop) : FSPoll(loop), stat_as_hash(false) {}
//protected:
//    void on_fs_poll (const stat_t* prev, const stat_t* curr, const CodeError* err) override;
//};
//
//struct XSTCP : TCP, XSStream {
//    XSTCP (Loop* loop = Loop::default_loop()) : TCP(loop) {}
//
//    StreamSP on_create_connection () override;
//
//    void open (const Sv& sv) {
//        if (!sv.is_ref()) return open((sock_t)SvUV(sv));
//        io_sv = sv;
//        TCP::open((sock_t)PerlIO_fileno(IoIFP(xs::in<IO*>(aTHX_ sv))));
//    }
//
//    void open (sock_t sock) override {
//        io_sv.reset();
//        TCP::open(sock);
//    }
//
//    template<class Builder>
//    static Builder construct_connect(SV* host_or_sa, SV* port_or_callback, float timeout, const AddrInfoHintsSP& hints, bool reconnect) {
//        if (port_or_callback && !SvROK(port_or_callback)) {
//            return Builder().to(xs::in<string>(host_or_sa), xs::in<uint16_t>(port_or_callback), hints).timeout(timeout).reconnect(reconnect);
//        } else {
//            return Builder().to(xs::in<SockAddr>(host_or_sa)).timeout(timeout).reconnect(reconnect);
//        }
//    }
//
//private:
//    Sv io_sv;
//};
//
//struct XSTTY : TTY, XSStream {
//    XSTTY (Sv io, bool readable = false, Loop* loop = Loop::default_loop()) : TTY(sv2file(io), readable, loop) {
//        if (io.is_ref()) io_sv = io;
//    }
//
//    XSTTY (file_t fd, bool readable = false, Loop* loop = Loop::default_loop()) : TTY(fd, readable, loop) {}
//
//    StreamSP on_create_connection () override;
//
//private:
//    Sv io_sv;
//
//};

}}

namespace xs {

template <> struct Typemap <panda::unievent::backend::Backend*> : TypemapObject<panda::unievent::backend::Backend*, panda::unievent::backend::Backend*, ObjectTypeForeignPtr, ObjectStorageMG> {
    panda::string package () { return "UniEvent::Backend"; }
};

template <class TYPE> struct Typemap <panda::unievent::Loop*, TYPE> : TypemapObject<panda::unievent::Loop*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {
    panda::string package () { return "UniEvent::Loop"; }
};

template <class TYPE> struct Typemap <panda::unievent::Handle*, TYPE> : TypemapObject<panda::unievent::Handle*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {};

template <class TYPE> struct Typemap <panda::unievent::Prepare*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::Prepare"; }
};

template <class TYPE> struct Typemap <panda::unievent::Check*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::Check"; }
};

template <class TYPE> struct Typemap <panda::unievent::Idle*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::Idle"; }
};

template <class TYPE> struct Typemap <panda::unievent::Timer*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::Timer"; }
};

template <class TYPE> struct Typemap <panda::unievent::Signal*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::Signal"; }
};

template <class TYPE> struct Typemap <panda::unievent::Udp*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::Udp"; }
};

template <class TYPE> struct Typemap <panda::unievent::Stream*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {};

template <class TYPE> struct Typemap <panda::unievent::Pipe*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    panda::string package () { return "UniEvent::Pipe"; }
};

//template <class TYPE> struct Typemap <panda::unievent::FSEvent*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
//    panda::string package () { return "UniEvent::FSEvent"; }
//};
//
//template <class TYPE> struct Typemap <panda::unievent::FSPoll*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
//    panda::string package () { return "UniEvent::FSPoll"; }
//};
//
//template <class TYPE> struct Typemap <panda::unievent::TCP*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
//    panda::string package () { return "UniEvent::TCP"; }
//};
//
//template <class TYPE> struct Typemap <panda::unievent::TTY*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
//    panda::string package () { return "UniEvent::TTY"; }
//};

}
