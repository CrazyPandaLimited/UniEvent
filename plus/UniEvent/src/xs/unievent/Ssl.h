#pragma once
#include <xs.h>
#include <panda/unievent/SslContext.h>

extern "C" {
    int SSL_CTX_up_ref(SSL_CTX *ctx);
    void SSL_CTX_free(SSL_CTX *ctx);
}

namespace xs {

template<>
struct Typemap<SSL_CTX*> : TypemapObject<SSL_CTX*, SSL_CTX*, ObjectTypePtr, ObjectStorageIV> {};

template <class TYPE>
struct Typemap<panda::unievent::SslContext, TYPE> : Typemap<SSL_CTX*> {
    using SslContext = panda::unievent::SslContext;

    static SslContext in(SV* arg) {
        if (!SvOK(arg)) return nullptr;
        if (SvROK(arg)) return Typemap<SSL_CTX*>::in(arg);
        return reinterpret_cast<SSL_CTX*>(SvIV(arg));
    }

    static Sv out (SslContext& context, const Sv& sv = Sv()) {
        SSL_CTX_up_ref(context);
        return Typemap<SSL_CTX*>::out(context, sv);
    }

    static void destroy (const SSL_CTX* ctx, SV*) {
        SSL_CTX_free((SSL_CTX*)ctx);
    }
};

}
