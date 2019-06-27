#pragma once
#include "Error.h"
#include "Handle.h"
#include <panda/unievent/FsEvent.h>

namespace xs { namespace unievent {

struct XSFsEvent : panda::unievent::FsEvent {
    using FsEvent::FsEvent;
protected:
    void on_fs_event (const std::string_view& file, int events, const panda::unievent::CodeError&) override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::FsEvent*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {
    static panda::string package () { return "UniEvent::FsEvent"; }
};

}
