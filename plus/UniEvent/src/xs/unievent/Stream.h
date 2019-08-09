#pragma once
#include "util.h"
#include "Error.h"
#include "Handle.h"
#include "Request.h"
#include <panda/unievent/Stream.h>

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
