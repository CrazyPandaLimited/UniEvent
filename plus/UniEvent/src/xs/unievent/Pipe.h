#pragma once
#include "Stream.h"
#include <panda/unievent/Pipe.h>

namespace xs { namespace unievent {

struct XSPipe : panda::unievent::Pipe, XSStream {
    using Pipe::Pipe;
    panda::unievent::StreamSP create_connection () override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Pipe*, TYPE> : Typemap<panda::unievent::Stream*, TYPE> {
    static panda::string package () { return "UniEvent::Pipe"; }
};

}
