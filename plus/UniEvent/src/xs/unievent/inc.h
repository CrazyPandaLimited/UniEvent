#pragma once
#include <xs.h>
#include <panda/unievent.h>
#include <xs/net/sockaddr.h>

namespace xs {

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

}
