#pragma once
#include <xs.h>
#include <panda/unievent.h>
#include <xs/net/sockaddr.h>

namespace xs {

template <class TYPE_PTR>
struct Typemap<addrinfo*, TYPE_PTR> : TypemapBase<addrinfo*, TYPE_PTR> {
    using TYPE = typename std::remove_pointer<TYPE_PTR>::type;
    static TYPE_PTR in (pTHX_ SV* arg) {
        if (!SvOK(arg)) return nullptr;
        if (!SvPOK(arg) || SvCUR(arg) < sizeof(TYPE)) throw "argument is not a valid addrinfo/in_addr/in6_addr";
        return reinterpret_cast<TYPE_PTR>(SvPVX(arg));
    }

    static Sv create (pTHX_ TYPE_PTR var, Sv = Sv()) {
        if (!var) return &PL_sv_undef;
        return Simple(std::string_view(reinterpret_cast<char*>(var), sizeof(TYPE)));
    }
};

template <> struct Typemap<SSL_CTX*> : TypemapBase<SSL_CTX*> {
    static SSL_CTX* in (pTHX_ SV* arg) {
        if (!SvOK(arg)) return nullptr;
        return reinterpret_cast<SSL_CTX*>(SvIV(arg));
    }
};

}
