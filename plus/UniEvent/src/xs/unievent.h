#pragma once
#include <xs.h>
#include <panda/unievent.h>
#include <xs/net/sockaddr.h>

#if PERL_VERSION < 28
    #undef gv_fetchmeth_sv
    #define gv_fetchmeth_sv(a,b,c,d) Perl_gv_fetchmeth_pvn(aTHX_ a,SvPV_nolen(b),SvCUR(b),c,d)
#endif

namespace xs { namespace unievent {

using namespace panda::unievent;

Stash get_perl_class_for_err    (const Error& err);
Stash get_perl_class_for_handle (Handle* h);

inline file_t sv2file (const Sv& sv) {
    if (sv.is_ref()) return (file_t)PerlIO_fileno(IoIFP( xs::in<IO*>(sv) ));
    else             return (file_t)SvUV(sv);
}

struct XSCallback {
    CV* callback;

    XSCallback () : callback(nullptr) {}

    void set  (SV* callback_rv);
    SV*  get  ();
    bool call (const Object& handle, const Simple& evname, std::initializer_list<Scalar> args = {});

    ~XSCallback () {
        SvREFCNT_dec(callback); // release callback
    }
};

//struct XSCommandCallback : panda::unievent::CommandCallback {
//    XSCallback xscb;
//    SV* handle_rv;
//
//    XSCommandCallback (pTHX_ SV* handle_rv) : CommandCallback(nullptr), handle_rv(handle_rv) {
//        SvREFCNT_inc_simple_void_NN(handle_rv);
//    }
//
//    virtual void run    ();
//    virtual void cancel ();
//
//    ~XSCommandCallback ();
//};

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
    void on_connection (const StreamSP&, const CodeError*) override;
    void on_connect    (const CodeError*, const ConnectRequestSP&) override;
    void on_read       (string&, const CodeError*) override;
//    void on_write      (const CodeError*, WriteRequest*) override;
    void on_shutdown   (const CodeError*, const ShutdownRequestSP&) override;
    void on_eof        () override;
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

template <class TYPE> struct Typemap<const panda::unievent::Error*, TYPE> : TypemapObject<const panda::unievent::Error*, TYPE, ObjectTypePtr, ObjectStorageMG> {
    using Super = TypemapObject<const panda::unievent::Error*, TYPE, ObjectTypePtr, ObjectStorageMG>;
    Sv create (pTHX_ TYPE var, const Sv& sv = {}) {
        if (!var) return Sv::undef;
        return Super::create(aTHX_ var->clone(), sv ? sv : xs::unievent::get_perl_class_for_err(*var));
    }
};

template <class TYPE> struct Typemap<const panda::unievent::CodeError*, TYPE> : Typemap<const panda::unievent::Error*,     TYPE> {};
template <class TYPE> struct Typemap<const panda::unievent::SSLError*,  TYPE> : Typemap<const panda::unievent::CodeError*, TYPE> {};

template <class TYPE> struct Typemap<const panda::unievent::Error&, TYPE&> : TypemapRefCast<TYPE&> {
    Sv out (pTHX_ TYPE& var, const Sv& proto = {}) { return Typemap<TYPE*>::out(aTHX_ &var, proto); }
};

template <> struct Typemap<panda::unievent::AddrInfoHints> : TypemapBase<panda::unievent::AddrInfoHints> {
    using Hints = panda::unievent::AddrInfoHints;

    Hints in (pTHX_ SV* arg);

    Sv create (pTHX_ const Hints& var, Sv = Sv()) {
        return Simple(std::string_view(reinterpret_cast<const char*>(&var), sizeof(Hints)));
    }
};

//template <> struct Typemap<SSL_CTX*> : TypemapBase<SSL_CTX*> {
//    SSL_CTX* in (pTHX_ SV* arg) {
//        if (!SvOK(arg)) return nullptr;
//        return reinterpret_cast<SSL_CTX*>(SvIV(arg));
//    }
//};

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

template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    std::string package () { return "UniEvent::Resolver"; }
};
template <class TYPE> struct Typemap<panda::unievent::Resolver::Request*, TYPE> : TypemapObject<panda::unievent::Resolver::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    std::string package () { return "UniEvent::Resolver::Request"; }
};

}

//namespace xs { namespace unievent {
//
//using xs::my_perl;
//using namespace panda::unievent;
//
//// The following classes are not visible from perl, XS wrappers are just needed for holding perl callback
//
//
//struct XSTCPConnectRequest : TCPConnectRequest {
//    XSCallback xscb;
//
//    XSTCPConnectRequest(
//        bool reconnect, const SockAddr& sa, const string& host, uint16_t port, const AddrInfoHintsSP& hints, uint64_t timeout, SV* xs_callback)
//            : TCPConnectRequest(reconnect, sa, host, port, hints, timeout, xs_callback ? _cb : connect_fn(nullptr)) {
//        xscb.set(xs_callback);
//    }
//
//    struct Builder : BasicBuilder<Builder> {
//        Builder& callback (SV* xs_callback) { _xs_callback = xs_callback; return *this; }
//
//        XSTCPConnectRequest* build () {
//            return new XSTCPConnectRequest(_reconnect, _sa, _host, _port, _hints, _timeout, _xs_callback);
//        }
//
//    private:
//        SV* _xs_callback = nullptr;
//    };
//
//private:
//    static void _cb (Stream* handle, const CodeError* err, ConnectRequest* req);
//};
//
//
//
//struct XSWriteRequest : WriteRequest {
//    XSCallback xscb;
//    XSWriteRequest (pTHX_ SV* callback) : WriteRequest(callback ? _cb : write_fn(nullptr)) {
//        xscb.set(callback);
//    }
//private:
//    static void _cb (Stream* handle, const CodeError* err, WriteRequest* req);
//};
//
//}}
