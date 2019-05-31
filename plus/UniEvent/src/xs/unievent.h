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

inline fd_t sv2file (const Sv& sv) {
    if (sv.is_ref()) return (fd_t)PerlIO_fileno(IoIFP( xs::in<IO*>(sv) ));
    else             return (fd_t)SvUV(sv);
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

struct XSPrepare : Prepare {
    using Prepare::Prepare;
    XSCallback prepare_xscb;
protected:
    void on_prepare () override;
};


struct XSCheck : Check {
    using Check::Check;
    XSCallback check_xscb;
protected:
    void on_check () override;
};


struct XSIdle : Idle {
    using Idle::Idle;
    XSCallback idle_xscb;
protected:
    void on_idle () override;
};


struct XSTimer : Timer {
    using Timer::Timer;
    XSCallback timer_xscb;
protected:
    void on_timer () override;
};


struct XSSignal : Signal {
    using Signal::Signal;
    XSCallback signal_xscb;
protected:
    void on_signal (int signum) override;
};


struct XSUdp : Udp {
    using Udp::Udp;
    XSCallback receive_xscb;
    XSCallback send_xscb;
protected:
    void on_receive (string& buf, const panda::net::SockAddr& sa, unsigned flags, const CodeError& err) override;
    void on_send    (const CodeError& err, const SendRequestSP& req) override;
};


struct XSStream : virtual Stream {
    using Stream::Stream;
    XSCallback connection_xscb;
    XSCallback read_xscb;
    XSCallback write_xscb;
    XSCallback shutdown_xscb;
    XSCallback eof_xscb;
    XSCallback connect_xscb;
    XSCallback create_connection_xscb;
protected:
    void on_connection (const StreamSP&, const CodeError&) override;
    void on_connect    (const CodeError&, const ConnectRequestSP&) override;
    void on_read       (string&, const CodeError&) override;
    void on_write      (const CodeError&, const WriteRequestSP&) override;
    void on_shutdown   (const CodeError&, const ShutdownRequestSP&) override;
    void on_eof        () override;
};

struct XSPipe : Pipe, XSStream {
    using Pipe::Pipe;
    StreamSP create_connection () override;
};


struct XSTcp : Tcp, XSStream {
    using Tcp::Tcp;
    StreamSP create_connection () override;
};


struct XSTty : Tty, XSStream {
    using Tty::Tty;
    StreamSP create_connection () override;
};


//struct XSFsPoll : FsPoll {
//    using FsPoll::FsPoll;
//    XSCallback fs_poll_xscb;
//protected:
//    void on_fs_poll (const Stat& prev, const Stat& cur, const CodeError& err) override;
//};


//struct XSFSEvent : FSEvent, XSHandle {
//    XSCallback fs_event_xscb;
//    XSFSEvent (Loop* loop) : FSEvent(loop) {}
//protected:
//    void on_fs_event (const char* filename, int events) override;
//};
//

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Error*, TYPE*> : TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG> {};
template <class TYPE> struct Typemap<const panda::unievent::Error&, TYPE&> : Typemap<panda::unievent::Error*, TYPE*> {
    using Super = Typemap<panda::unievent::Error*, TYPE*>;

    static Sv out (pTHX_ TYPE& var, const Sv& sv = {}) {
        if (!var) return Sv::undef;
        return Super::out(aTHX_ var.clone(), sv ? sv : xs::unievent::get_perl_class_for_err(var));
    }

    static TYPE& in (pTHX_ const Sv& sv) {
        if (!sv.defined()) {
            static TYPE ret;
            return ret;
        }
        return *Super::in(aTHX_ sv);
    }
};

template <class TYPE> struct Typemap<panda::unievent::CodeError*, TYPE>  : Typemap<panda::unievent::Error*,     TYPE> {};
template <class TYPE> struct Typemap<panda::unievent::SSLError*,  TYPE>  : Typemap<panda::unievent::CodeError*, TYPE> {};

template <class TYPE> struct Typemap<const panda::unievent::CodeError&, TYPE&> : Typemap<const panda::unievent::Error&,     TYPE&> {};
template <class TYPE> struct Typemap<const panda::unievent::SSLError&,  TYPE&> : Typemap<const panda::unievent::CodeError&, TYPE&> {};

template <> struct Typemap<panda::unievent::AddrInfoHints> : TypemapBase<panda::unievent::AddrInfoHints> {
    using Hints = panda::unievent::AddrInfoHints;

    static Hints in (pTHX_ SV* arg);

    static Sv create (pTHX_ const Hints& var, Sv = Sv()) {
        return Simple(std::string_view(reinterpret_cast<const char*>(&var), sizeof(Hints)));
    }
};

template <> struct Typemap<SSL_CTX*> : TypemapBase<SSL_CTX*> {
    static SSL_CTX* in (pTHX_ SV* arg) {
        if (!SvOK(arg)) return nullptr;
        return reinterpret_cast<SSL_CTX*>(SvIV(arg));
    }
};

template <> struct Typemap<panda::unievent::Fs::Stat> : TypemapBase<panda::unievent::Fs::Stat> {
    static Sv out (pTHX_ const panda::unievent::Fs::Stat&, const Sv& = Sv());
    static panda::unievent::Fs::Stat in (pTHX_ const Array&);
};

template <> struct Typemap<panda::unievent::Fs::DirEntry> : TypemapBase<panda::unievent::Fs::DirEntry> {
    static Sv out (pTHX_ const panda::unievent::Fs::DirEntry&, const Sv& = Sv());
    static panda::unievent::Fs::DirEntry in (pTHX_ const Array&);
};

template <> struct Typemap<panda::unievent::backend::Backend*> : TypemapObject<panda::unievent::backend::Backend*, panda::unievent::backend::Backend*, ObjectTypeForeignPtr, ObjectStorageMG> {
    static panda::string package () { return "UniEvent::Backend"; }
};

template <class TYPE> struct Typemap<panda::unievent::Loop*, TYPE> : TypemapObject<panda::unievent::Loop*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {
    static panda::string package () { return "UniEvent::Loop"; }
};

template <class TYPE> struct Typemap<panda::unievent::Handle*, TYPE> : TypemapObject<panda::unievent::Handle*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {};

template <class TYPE> struct Typemap<panda::unievent::BackendHandle*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {};

template <class TYPE> struct Typemap<panda::unievent::Prepare*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Prepare"; }
};

template <class TYPE> struct Typemap<panda::unievent::Check*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Check"; }
};

template <class TYPE> struct Typemap<panda::unievent::Idle*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Idle"; }
};

template <class TYPE> struct Typemap<panda::unievent::Timer*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Timer"; }
};

template <class TYPE> struct Typemap<panda::unievent::Signal*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Signal"; }
};

template <class TYPE> struct Typemap<panda::unievent::Udp*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Udp"; }
};

template <class TYPE> struct Typemap<panda::unievent::Stream*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {};

template <class TYPE> struct Typemap<panda::unievent::Pipe*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Pipe"; }
};

template <class TYPE> struct Typemap<panda::unievent::Tcp*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Tcp"; }
};

template <class TYPE> struct Typemap<panda::unievent::Tty*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Tty"; }
};

template <class TYPE> struct Typemap<panda::unievent::Fs::Request*, TYPE> : TypemapObject<panda::unievent::Fs::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {
    static panda::string package () { return "UniEvent::Fs::Request"; }
};

template <class TYPE> struct Typemap<panda::unievent::FsPoll*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    static panda::string package () { return "UniEvent::FsPoll"; }
};

//template <class TYPE> struct Typemap<panda::unievent::FSEvent*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
//    static panda::string package () { return "UniEvent::FSEvent"; }
//};

template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    static std::string package () { return "UniEvent::Resolver"; }
};
template <class TYPE> struct Typemap<panda::unievent::Resolver::Request*, TYPE> : TypemapObject<panda::unievent::Resolver::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    static std::string package () { return "UniEvent::Resolver::Request"; }
};

}
