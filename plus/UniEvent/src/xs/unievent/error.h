#pragma once
#include <xs.h>
#include <panda/unievent/Error.h>

namespace xs { namespace unievent {

using panda::unievent::Error;
using panda::unievent::CodeError;
using xs::Scalar;

// errors are rare so use dTHX as perfomance is not a concern here.

Scalar error_sv (const Error& err, bool with_mess = false);

inline Scalar error_sv (const CodeError& err, bool with_mess = false) {
    if (err) return error_sv(static_cast<const Error&>(err), with_mess);
    return Scalar::undef;
}

}}
