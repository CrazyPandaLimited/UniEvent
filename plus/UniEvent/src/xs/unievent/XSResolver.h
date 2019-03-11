#pragma once
#include "inc.h"
#include <panda/unievent/Resolver.h>

namespace xs {
    template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
        std::string package () { return "UniEvent::Resolver"; }
    };
    template <class TYPE> struct Typemap<panda::unievent::Resolver::Request*, TYPE> : TypemapObject<panda::unievent::Resolver::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
        std::string package () { return "UniEvent::Resolver::Request"; }
    };
}
