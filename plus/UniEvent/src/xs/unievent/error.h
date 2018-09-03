#pragma once
#include <xs.h>
#include <panda/unievent/Error.h>

namespace xs { namespace unievent {

using panda::unievent::Error;
using panda::unievent::CodeError;
using xs::Scalar;

extern const CodeError code_error_def;

Stash get_perl_class_for_err (const Error& err);

Scalar error_sv (const Error& err, bool with_mess = false);

inline Scalar error_sv (const CodeError& err, bool with_mess = false) {
    if (err) return error_sv(static_cast<const Error&>(err), with_mess);
    return Scalar::undef;
}

}}

namespace xs {

    template <class TYPE> struct Typemap<panda::unievent::Error*, TYPE> : TypemapObject<panda::unievent::Error*, TYPE, ObjectTypePtr, ObjectStorageMG> {
        using Super = TypemapObject<panda::unievent::Error*, TYPE, ObjectTypePtr, ObjectStorageMG>;
        Sv create (pTHX_ TYPE var, const Sv& sv = {}) {
            if (!var) return Sv::undef;
            return Super::create(aTHX_ var, sv ? sv : xs::unievent::get_perl_class_for_err(*var));
        }
    };
    template <class TYPE> struct Typemap<panda::unievent::ImplRequiredError*, TYPE> : Typemap<panda::unievent::Error*,     TYPE> {};
    template <class TYPE> struct Typemap<panda::unievent::CodeError*,         TYPE> : Typemap<panda::unievent::Error*,     TYPE> {};
    template <class TYPE> struct Typemap<panda::unievent::SSLError*,          TYPE> : Typemap<panda::unievent::CodeError*, TYPE> {};

    template <> struct Typemap<const panda::unievent::CodeError&> : Typemap<panda::unievent::CodeError*> {
        using Super = Typemap<panda::unievent::CodeError*>;

        const panda::unievent::CodeError& in (pTHX_ SV* sv) {
            if (!SvOK(sv)) return xs::unievent::code_error_def;
            return *Super::in(aTHX_ sv);
        }

        Sv create (pTHX_ const panda::unievent::CodeError& var, const Sv& sv = {}) {
            if (!var) return Sv::undef;
            return Super::create(aTHX_ var.clone(), sv);
        }

    };
}
