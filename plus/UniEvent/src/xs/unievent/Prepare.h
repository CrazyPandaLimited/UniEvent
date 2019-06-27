#pragma once
#include "Handle.h"
#include <panda/unievent/Prepare.h>

namespace xs { namespace unievent {

struct XSPrepare : panda::unievent::Prepare {
    using Prepare::Prepare;
protected:
    void on_prepare () override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Prepare*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Prepare"; }
};

}
