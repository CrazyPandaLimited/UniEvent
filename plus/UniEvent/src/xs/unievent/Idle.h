#pragma once
#include "Handle.h"
#include <panda/unievent/Idle.h>

namespace xs { namespace unievent {

struct XSIdle : panda::unievent::Idle {
    using Idle::Idle;
protected:
    void on_idle () override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Idle*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Idle"; }
};

}
