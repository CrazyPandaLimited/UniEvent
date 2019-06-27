#pragma once
#include "Handle.h"
#include <panda/unievent/Check.h>

namespace xs { namespace unievent {

struct XSCheck : panda::unievent::Check {
    using Check::Check;
protected:
    void on_check () override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Check*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Check"; }
};

}
