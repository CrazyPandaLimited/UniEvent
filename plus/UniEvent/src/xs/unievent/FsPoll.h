#pragma once
#include "Fs.h"
#include "Error.h"
#include "Handle.h"
#include <panda/unievent/FsPoll.h>

namespace xs { namespace unievent {

struct XSFsPoll : panda::unievent::FsPoll {
    using FsPoll::FsPoll;
protected:
    void on_fs_poll (const panda::unievent::Fs::Stat& prev, const panda::unievent::Fs::Stat& cur, const panda::unievent::CodeError& err) override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::FsPoll*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    static panda::string package () { return "UniEvent::FsPoll"; }
};

}
