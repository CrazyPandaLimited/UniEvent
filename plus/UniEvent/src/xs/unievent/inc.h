#pragma once
#include <xs.h>
#include <panda/unievent.h>

namespace xs {

template <class TYPE_PTR>
struct Typemap<addrinfo*, TYPE_PTR> : TypemapBase<addrinfo*, TYPE_PTR> {
    using TYPE = typename std::remove_pointer<TYPE_PTR>::type;
    TYPE_PTR in (pTHX_ SV* arg) {
        if (!SvOK(arg)) return nullptr;
        if (!SvPOK(arg) || SvCUR(arg) < sizeof(TYPE)) throw "argument is not a valid addrinfo/in_addr/in6_addr";
        return reinterpret_cast<TYPE_PTR>(SvPVX(arg));
    }

    Sv create (pTHX_ TYPE_PTR var, Sv = Sv()) {
        if (!var) return &PL_sv_undef;
        return Simple(std::string_view(reinterpret_cast<char*>(var), sizeof(TYPE)));
    }
};

template <> struct Typemap<in_addr* > : Typemap<addrinfo*, in_addr* > {};
template <> struct Typemap<in6_addr*> : Typemap<addrinfo*, in6_addr*> {};

template <>
struct Typemap<sockaddr*> : TypemapBase<sockaddr*> {
    sockaddr* in (pTHX_ SV* arg) {
        if (!SvOK(arg)) return nullptr;
        if (!SvPOK(arg) || SvCUR(arg) < sizeof(sockaddr)) throw "argument is not a sockaddr";
        auto ret = reinterpret_cast<sockaddr*>(SvPVX(arg));
        if (ret->sa_family == PF_INET6 && SvCUR(arg) < sizeof(sockaddr_in6)) throw "argument is not a sockaddr";
        return ret;
    }

    Sv create (pTHX_ sockaddr* var, Sv = Sv()) {
        if (!var) return &PL_sv_undef;
        return Simple(std::string_view(
            reinterpret_cast<char*>(var),
            var->sa_family == PF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in)
        ));
    }
};

template <> struct Typemap<SSL_CTX*> : TypemapBase<SSL_CTX*> {
    SSL_CTX* in (pTHX_ SV* arg) {
        if (!SvOK(arg)) return nullptr;
        return reinterpret_cast<SSL_CTX*>(SvIV(arg));
    }
};

}
