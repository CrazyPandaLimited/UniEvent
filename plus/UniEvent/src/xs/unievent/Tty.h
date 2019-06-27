#pragma once
#include "Stream.h"
#include <panda/unievent/Tty.h>

namespace xs { namespace unievent {

struct XSTty : panda::unievent::Tty, XSStream {
    using Tty::Tty;
    panda::unievent::StreamSP create_connection () override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Tty*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Tty"; }
};

}
