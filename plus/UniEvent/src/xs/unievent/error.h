#pragma once
#include <xs.h>
#include <panda/unievent/error.h>

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Error*, TYPE*> : TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG> {
    static panda::string_view package () { return "UniEvent::Error"; }
};

}

#define UE_EXCEPTED_VOID(code)  do {                         \
    auto ret = code;                                        \
    if (GIMME_V == G_VOID) {                                \
        if (!ret) panda::exthrow(ret.error());              \
        XSRETURN_EMPTY;                                     \
    }                                                       \
    XPUSHs(boolSV(ret));                                    \
    if (GIMME_V == G_ARRAY) {                               \
        if (ret) XPUSHs(&PL_sv_undef);                      \
        else     mXPUSHs(xs::out(ret.error()).detach());    \
        XSRETURN(2);                                        \
    }                                                       \
    XSRETURN(1);                                            \
} while(false)
