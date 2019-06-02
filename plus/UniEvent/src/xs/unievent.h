#pragma once
#include <xs.h>
#include <panda/unievent.h>
#include <xs/net/sockaddr.h>

#if PERL_VERSION < 28
    #undef gv_fetchmeth_sv
    #define gv_fetchmeth_sv(a,b,c,d) Perl_gv_fetchmeth_pvn(aTHX_ a,SvPV_nolen(b),SvCUR(b),c,d)
#endif

namespace xs { namespace unievent {

Stash get_perl_class_for_err    (const panda::unievent::Error& err);
Stash get_perl_class_for_handle (panda::unievent::Handle* h);

inline panda::unievent::fd_t sv2fd (const Sv& sv) {
    if (sv.is_ref()) return (panda::unievent::fd_t)PerlIO_fileno(IoIFP( xs::in<IO*>(sv) ));
    else             return (panda::unievent::fd_t)SvUV(sv);
}

struct XSPrepare : panda::unievent::Prepare {
    using Prepare::Prepare;
protected:
    void on_prepare () override;
};


struct XSCheck : panda::unievent::Check {
    using Check::Check;
protected:
    void on_check () override;
};


struct XSIdle : panda::unievent::Idle {
    using Idle::Idle;
protected:
    void on_idle () override;
};


struct XSTimer : panda::unievent::Timer {
    using Timer::Timer;
protected:
    void on_timer () override;
};


struct XSSignal : panda::unievent::Signal {
    using Signal::Signal;
protected:
    void on_signal (int signum) override;
};


struct XSUdp : panda::unievent::Udp {
    using Udp::Udp;
protected:
    void on_receive (panda::string& buf, const panda::net::SockAddr& sa, unsigned flags, const panda::unievent::CodeError& err) override;
    void on_send    (const panda::unievent::CodeError& err, const panda::unievent::SendRequestSP& req) override;
};


struct XSStream : virtual panda::unievent::Stream {
    using Stream::Stream;
protected:
    void on_connection (const panda::unievent::StreamSP&, const panda::unievent::CodeError&) override;
    void on_connect    (const panda::unievent::CodeError&, const panda::unievent::ConnectRequestSP&) override;
    void on_read       (panda::string&, const panda::unievent::CodeError&) override;
    void on_write      (const panda::unievent::CodeError&, const panda::unievent::WriteRequestSP&) override;
    void on_shutdown   (const panda::unievent::CodeError&, const panda::unievent::ShutdownRequestSP&) override;
    void on_eof        () override;
};

struct XSPipe : panda::unievent::Pipe, XSStream {
    using Pipe::Pipe;
    panda::unievent::StreamSP create_connection () override;
};


struct XSTcp : panda::unievent::Tcp, XSStream {
    using Tcp::Tcp;
    panda::unievent::StreamSP create_connection () override;
};


struct XSTty : panda::unievent::Tty, XSStream {
    using Tty::Tty;
    panda::unievent::StreamSP create_connection () override;
};


struct XSFsPoll : panda::unievent::FsPoll {
    using FsPoll::FsPoll;
protected:
    void on_fs_poll (const panda::unievent::Fs::Stat& prev, const panda::unievent::Fs::Stat& cur, const panda::unievent::CodeError& err) override;
};


struct XSFsEvent : panda::unievent::FsEvent {
    using FsEvent::FsEvent;
protected:
    void on_fs_event (const std::string_view& file, int events, const panda::unievent::CodeError&) override;
};


}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Error*, TYPE*> : TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG> {
    using Super = TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG>;
    static Sv out (pTHX_ TYPE* var, const Sv& sv = {}) {
        if (!var) return Sv::undef;
        return Super::out(aTHX_ var, sv ? sv : xs::unievent::get_perl_class_for_err(*var));
    }
};

template <class TYPE> struct Typemap<const panda::unievent::Error&, TYPE&> : Typemap<panda::unievent::Error*, TYPE*> {
    using Super = Typemap<panda::unievent::Error*, TYPE*>;

    static Sv out (pTHX_ TYPE& var, const Sv& sv = {}) {
        if (!var) return Sv::undef;
        return Super::out(aTHX_ var.clone(), sv);
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

template <class TYPE> struct Typemap<panda::unievent::Request*, TYPE> : TypemapObject<panda::unievent::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMG> {};

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

template <class TYPE> struct Typemap<panda::unievent::SendRequest*, TYPE> : Typemap<panda::unievent::Request*, TYPE> {
    static panda::string package () { return "UniEvent::Request::Send"; }
};

template <class TYPE> struct Typemap<panda::unievent::Stream*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {};

template <class TYPE> struct Typemap<panda::unievent::ConnectRequest*, TYPE> : Typemap<panda::unievent::Request*, TYPE> {
    static panda::string package () { return "UniEvent::Request::Connect"; }
};

template <class TYPE> struct Typemap<panda::unievent::WriteRequest*, TYPE> : Typemap<panda::unievent::Request*, TYPE> {
    static panda::string package () { return "UniEvent::Request::Write"; }
};

template <class TYPE> struct Typemap<panda::unievent::ShutdownRequest*, TYPE> : Typemap<panda::unievent::Request*, TYPE> {
    static panda::string package () { return "UniEvent::Request::Shutdown"; }
};

template <class TYPE> struct Typemap<panda::unievent::Pipe*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Pipe"; }
};

template <class TYPE> struct Typemap<panda::unievent::Tcp*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Tcp"; }
};

template <class TYPE> struct Typemap<panda::unievent::Tty*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Tty"; }
};

template <class TYPE> struct Typemap<panda::unievent::Fs::Request*, TYPE> : TypemapObject<panda::unievent::Fs::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    static panda::string package () { return "UniEvent::Fs::Request"; }
};

template <class TYPE> struct Typemap<panda::unievent::FsPoll*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    static panda::string package () { return "UniEvent::FsPoll"; }
};

template <class TYPE> struct Typemap<panda::unievent::FsEvent*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    static panda::string package () { return "UniEvent::FsEvent"; }
};

template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    static std::string package () { return "UniEvent::Resolver"; }
};
template <class TYPE> struct Typemap<panda::unievent::Resolver::Request*, TYPE> : TypemapObject<panda::unievent::Resolver::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    static std::string package () { return "UniEvent::Resolver::Request"; }
};

}
