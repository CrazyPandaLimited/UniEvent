#pragma once
#include <xs.h>
#include <panda/unievent/Error.h>

namespace xs { namespace unievent {

Stash get_perl_class_for_err (const panda::unievent::Error& err);

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Error*, TYPE*> : TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG> {
    using Super = TypemapObject<panda::unievent::Error*, TYPE*, ObjectTypePtr, ObjectStorageMG>;
    static Sv out (pTHX_ TYPE* var, const Sv& sv = {}) {
        if (!var) return Sv::undef;
        return Super::out(aTHX_ var, sv ? sv : xs::unievent::get_perl_class_for_err(*var));
    }
};
template <class TYPE> struct Typemap<panda::unievent::CodeError*, TYPE>  : Typemap<panda::unievent::Error*,     TYPE> {};
template <class TYPE> struct Typemap<panda::unievent::SSLError*,  TYPE>  : Typemap<panda::unievent::CodeError*, TYPE> {};

template <class TYPE> struct Typemap<const panda::unievent::Error&, TYPE&> : Typemap<panda::unievent::Error*, TYPE*> {
    using Super = Typemap<panda::unievent::Error*, TYPE*>;

    static Sv out (pTHX_ TYPE& var, const Sv& sv = {}) { return Super::out(aTHX_ var.clone(), sv); }

    static TYPE& in (pTHX_ const Sv& sv) {
        auto ret = Super::in(aTHX_ sv);
        if (!ret) throw "error must not be undef here";
        return *ret;
    }
};

template <class TYPE> struct Typemap<const panda::unievent::CodeError&, TYPE&> : Typemap<const panda::unievent::Error&, TYPE&> {
    using Super = Typemap<const panda::unievent::Error&, TYPE&>;

    static Sv out (pTHX_ TYPE& var, const Sv& sv = {}) { return var ? Super::out(aTHX_ var, sv) : Sv::undef; }

    static TYPE& in (pTHX_ const Sv& sv) {
        if (!sv.defined()) {
            static TYPE ret;
            return ret;
        }
        return Super::in(aTHX_ sv);
    }
};

template <class TYPE> struct Typemap<const panda::unievent::SSLError&, TYPE&> : Typemap<const panda::unievent::CodeError&, TYPE&> {};

}
