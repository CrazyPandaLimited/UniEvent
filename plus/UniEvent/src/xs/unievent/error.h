#pragma once
#include <xs.h>
#include <panda/unievent/Error.h>

namespace xs { namespace unievent {

using xs::Stash;
using panda::unievent::Error;

Stash get_perl_class_for_err (const Error& err);

}}

static const panda::unievent::Error noerr;

namespace xs {
    template <class TYPE> struct Typemap<panda::unievent::Error*, TYPE*> : TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG> {
        using Super = TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG>;
        static Sv create (pTHX_ TYPE* var, const Sv& sv = {}) {
            if (!var) return Sv::undef;
            return Super::create(aTHX_ var->clone(), sv ? sv : xs::unievent::get_perl_class_for_err(*var));
        }
    };
    template <class TYPE> struct Typemap<panda::unievent::ImplRequiredError*, TYPE> : Typemap<panda::unievent::Error*,     TYPE> {};
    template <class TYPE> struct Typemap<panda::unievent::CodeError*,         TYPE> : Typemap<panda::unievent::Error*,     TYPE> {};
    template <class TYPE> struct Typemap<panda::unievent::SSLError*,          TYPE> : Typemap<panda::unievent::CodeError*, TYPE> {};

    template <class TYPE> struct Typemap<const panda::unievent::Error&, TYPE&> : Typemap<panda::unievent::Error*, TYPE*> {
        using Super = Typemap<panda::unievent::Error*, TYPE*>;
        static TYPE& in (pTHX_ SV* arg) {
            auto ret = Super::in(aTHX_ arg);
            if (!ret) return noerr;
            return *ret;
        }
        static Sv out (pTHX_ TYPE& var, const Sv& sv = {}) { return Super::out(aTHX_ &var, sv); }
    };
}
