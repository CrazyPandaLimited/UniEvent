#pragma once
#include <xs.h>
#include <panda/unievent/Error.h>

namespace xs { namespace unievent {

using xs::Stash;
using panda::unievent::Error;

Stash get_perl_class_for_err (const Error& err);

}}

namespace xs {

    template <class TYPE> struct Typemap<const panda::unievent::Error*, TYPE> : TypemapObject<const panda::unievent::Error*, TYPE, ObjectTypePtr, ObjectStorageMG> {
        using Super = TypemapObject<const panda::unievent::Error*, TYPE, ObjectTypePtr, ObjectStorageMG>;
        Sv create (pTHX_ TYPE var, const Sv& sv = {}) {
            if (!var) return Sv::undef;
            return Super::create(aTHX_ var->clone(), sv ? sv : xs::unievent::get_perl_class_for_err(*var));
        }
    };
    template <class TYPE> struct Typemap<const panda::unievent::ImplRequiredError*, TYPE> : Typemap<const panda::unievent::Error*,     TYPE> {};
    template <class TYPE> struct Typemap<const panda::unievent::CodeError*,         TYPE> : Typemap<const panda::unievent::Error*,     TYPE> {};
//    template <class TYPE> struct Typemap<const panda::unievent::SSLError*,          TYPE> : Typemap<const panda::unievent::CodeError*, TYPE> {};

    template <class TYPE> struct Typemap<const panda::unievent::Error&, TYPE&> : TypemapRefCast<TYPE&> {
        Sv out (pTHX_ TYPE& var, const Sv& proto = {}) { return Typemap<TYPE*>::out(aTHX_ &var, proto); }
    };
    template <class TYPE> struct Typemap<const panda::unievent::ImplRequiredError&, TYPE> : Typemap<const panda::unievent::Error&, TYPE> {};

}
