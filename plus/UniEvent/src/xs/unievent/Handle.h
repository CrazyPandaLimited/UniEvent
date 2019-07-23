#pragma once
#include "Loop.h"
#include <panda/unievent/BackendHandle.h>

namespace xs { namespace unievent {

Stash perl_class_for_handle (panda::unievent::Handle* h);

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Handle*, TYPE> : TypemapObject<panda::unievent::Handle*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {};

template <class TYPE> struct Typemap<panda::unievent::BackendHandle*, TYPE> : Typemap<panda::unievent::Handle*, TYPE> {};

}
