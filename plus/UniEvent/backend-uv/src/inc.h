#pragma once
#include <uv.h>
#include <panda/unievent/util.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/Error.h>
#include <panda/unievent/backend/BackendHandle.h>

#define uvx_status2err(status) (status ? static_cast<const CodeError*>(uvx_code_error(status)) : nullptr)

namespace panda { namespace unievent { namespace backend { namespace uv {

template <class T = BackendHandle*, class X>
inline T get_handle (X* uvhp) {
    return static_cast<T>(reinterpret_cast<BackendHandle*>(uvhp->data));
}

}}}}
