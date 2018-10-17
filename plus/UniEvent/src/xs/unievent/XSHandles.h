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

struct XSHandle : virtual Handle {
protected:
    mTHX;
    XSHandle () { aTHXa(PERL_GET_THX); }
};


struct XSPrepare : Prepare, XSHandle {
    XSCallback prepare_xscb;
    XSPrepare (Loop* loop) : Prepare(loop) {}
protected:
    void on_prepare () override;
};


struct XSCheck : Check, XSHandle {
    XSCallback check_xscb;
    XSCheck (Loop* loop) : Check(loop) {}
protected:
    void on_check () override;
};


struct XSIdle : Idle, XSHandle {
    XSCallback idle_xscb;
    XSIdle (Loop* loop) : Idle(loop) {}
protected:
    void on_idle () override;
};


struct XSTimer : Timer, XSHandle {
    XSCallback timer_xscb;
    XSTimer (Loop* loop) : Timer(loop) {}
protected:
    void on_timer () override;
};


struct XSFSEvent : FSEvent, XSHandle {
    XSCallback fs_event_xscb;
    XSFSEvent (Loop* loop) : FSEvent(loop) {}
protected:
    void on_fs_event (const char* filename, int events) override;
};


struct XSFSPoll : FSPoll, XSHandle {
    XSCallback fs_poll_xscb;
    bool       stat_as_hash;
    XSFSPoll (Loop* loop) : FSPoll(loop), stat_as_hash(false) {}
protected:
    void on_fs_poll (const stat_t* prev, const stat_t* curr, const CodeError* err) override;
};


struct XSSignal : Signal, XSHandle {
    XSCallback signal_xscb;
    XSSignal (Loop* loop) : Signal(loop) {}
protected:
    void on_signal (int signum) override;
};


struct XSStream : virtual Stream, XSHandle {
    XSCallback connection_xscb;
    XSCallback read_xscb;
    XSCallback write_xscb;
    XSCallback shutdown_xscb;
    XSCallback eof_xscb;
    XSCallback connect_xscb;
    XSCallback create_connection_xscb;

    XSStream () {}

protected:
    void on_connection (Stream*, const CodeError*) override;
    void on_connect    (const CodeError*, ConnectRequest*) override;
    void on_read       (string&, const CodeError*) override;
    void on_write      (const CodeError*, WriteRequest*) override;
    void on_shutdown   (const CodeError*, ShutdownRequest*) override;
    void on_eof        () override;
};

struct XSTCP : TCP, XSStream {
    XSTCP (Loop* loop = Loop::default_loop()) : TCP(loop) {
        connection_factory = [=](){
            TCPSP ret = make_backref<XSTCP>(loop);
            xs::out<TCP*>(ret.get());
            return ret;
        };
    }

    void open (const Sv& sv) {
        if (!sv.is_ref()) return open((sock_t)SvUV(sv));
        io_sv = sv;
        TCP::open((sock_t)PerlIO_fileno(IoIFP(xs::in<IO*>(aTHX_ sv))));
    }

    void open (sock_t sock) override {
        io_sv.reset();
        TCP::open(sock);
    }
    template<class Builder>
    static Builder construct_connect(SV* host_or_sa, SV* service_or_callback, float timeout, addrinfo* hints, bool reconnect) {
        if (service_or_callback && !SvROK(service_or_callback)) {
            return Builder().to(xs::in<string>(host_or_sa), xs::in<string>(service_or_callback), hints).timeout(timeout).reconnect(reconnect);
        } else {
            return Builder().to(xs::in<sockaddr*>(host_or_sa)).timeout(timeout).reconnect(reconnect);
        }
    }

private:
    Sv io_sv;
};

struct XSUDP : UDP, XSHandle {
    XSCallback receive_xscb;
    XSCallback send_xscb;

    XSUDP (Loop * loop = Loop::default_loop()) : UDP(loop) {
        flags |= XUF_DONTRECV;
    }
    
    void bind (const SockAddr& sa, unsigned flags = 0) override {
        UDP::bind(sa, flags | get_bind_flags());
    }
    
    void bind (string_view host, string_view service, const addrinfo* hints = nullptr, unsigned flags = 0) override {
        UDP::bind(host, service, hints, flags | get_bind_flags());
    }

    void open (const Sv& sv) {
        if (!sv.is_ref()) return open((sock_t)SvUV(sv));
        io_sv = sv;
        UDP::open((sock_t)PerlIO_fileno(IoIFP(xs::in<IO*>(aTHX_ sv))));
    }

    void open (sock_t sock) override {
        io_sv.reset();
        UDP::open(sock);
    }

    void so_reuseaddr (bool new_val) {
        flags ^= (flags ^ -new_val) & XUF_REUSEADDR;
    }

    bool so_reuseaddr () {
        return flags & XUF_REUSEADDR;
    }

    bool want_recv () {
        return !(flags & XUF_DONTRECV);
    }

    void want_recv (bool want) {
        if (want_recv() != want) {
            if (want) {
                recv_start();
            }
            else {
                recv_stop();
            }
            flags ^= (flags ^ -!want) & XUF_DONTRECV;
        }
    }
    
protected:
    void on_receive (string& buf, const sockaddr* sa, unsigned flags, const CodeError* err) override;
    void on_send    (const CodeError* err, SendRequest* req) override;

private:
    static const int XUF_REUSEADDR = UF_LAST << 1;
    static const int XUF_DONTRECV = XUF_REUSEADDR << 1;

    Sv io_sv;

    unsigned get_bind_flags () {
        return (-( ( XUF_REUSEADDR & flags ) != 0)) & UV_UDP_REUSEADDR;
    }
};

struct XSPipe : Pipe, XSStream {
    XSPipe (bool ipc, Loop* loop) : Pipe(ipc, loop) {
        connection_factory = [=](){
            PipeSP ret = make_backref<XSPipe>(ipc, loop);
            xs::out<Pipe*>(ret.get());
            return ret;
        };
    }

    void open (const Sv& sv) {
        if (!sv.is_ref()) return open((sock_t)SvUV(sv));
        io_sv = sv;
        Pipe::open((file_t)PerlIO_fileno(IoIFP(xs::in<IO*>(aTHX_ sv))));
    }

    void open (file_t sock) override {
        io_sv.reset();
        Pipe::open(sock);
    }

private:
    Sv io_sv;
};

struct XSTTY : TTY, XSStream {
    XSTTY (Sv io, bool readable = false, Loop* loop = Loop::default_loop()) : TTY(sv2file(io), readable, loop) {
        if (io.is_ref()) io_sv = io;
    }
    
private:
    Sv io_sv;

    static file_t sv2file (const Sv& sv) {
        dTHX;
        if (sv.is_ref()) return (file_t)PerlIO_fileno(IoIFP( xs::in<IO*>(sv) ));
        else             return (file_t)SvUV(sv);
    }
};

}}

namespace xs {

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

template <class TYPE> struct Typemap <panda::unievent::FSEvent*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::FSEvent"; }
};

template <class TYPE> struct Typemap <panda::unievent::FSPoll*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::FSPoll"; }
};

template <class TYPE> struct Typemap <panda::unievent::Signal*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::Signal"; }
};

template <class TYPE> struct Typemap <panda::unievent::Stream*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {};

template <class TYPE> struct Typemap <panda::unievent::TCP*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    panda::string package () { return "UniEvent::TCP"; }
};

template <class TYPE> struct Typemap <panda::unievent::Pipe*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    panda::string package () { return "UniEvent::Pipe"; }
};

template <class TYPE> struct Typemap <panda::unievent::TTY*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    panda::string package () { return "UniEvent::TTY"; }
};

template <class TYPE> struct Typemap <panda::unievent::UDP*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    panda::string package () { return "UniEvent::UDP"; }
};

}
