#pragma once
#include "util.h"
#include "Error.h"
#include "Handle.h"
#include "Request.h"
#include <panda/unievent/Stream.h>

namespace xs { namespace unievent {

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

}}

namespace xs {

template <> struct Typemap<SSL_CTX*> : TypemapBase<SSL_CTX*> {
    static SSL_CTX* in (pTHX_ SV* arg) {
        if (!SvOK(arg)) return nullptr;
        return reinterpret_cast<SSL_CTX*>(SvIV(arg));
    }
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

}
