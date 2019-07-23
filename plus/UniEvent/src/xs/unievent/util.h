#pragma once
#include <xs.h>
#include <panda/unievent/inc.h>

namespace xs { namespace unievent {

inline panda::unievent::fd_t sv2fd (const Sv& sv) {
    if (sv.is_ref()) return (panda::unievent::fd_t)PerlIO_fileno(IoIFP( xs::in<IO*>(sv) ));
    else             return (panda::unievent::fd_t)SvUV(sv);
}

inline panda::unievent::sock_t sv2sock (const Sv& sv) { return (panda::unievent::sock_t)sv2fd(sv); }

panda::string sv2buf (const Sv& sv);

}}
