#pragma once
#include "Handle.h"
#include <panda/unievent/Signal.h>

namespace xs { namespace unievent {

struct XSSignal : panda::unievent::Signal {
    using Signal::Signal;
protected:
    void on_signal (int signum) override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Signal*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Signal"; }
};

}
