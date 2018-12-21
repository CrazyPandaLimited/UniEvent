#pragma once
#include <uv.h>
#include <panda/unievent/Error.h>
#include <panda/unievent/backend/BackendHandle.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

inline CodeError uvx_code_error (int uverr) {
    assert(uverr);
    return CodeError(
        std::make_error_code((std::errc)(-uverr)),
        string(uv_err_name(uverr)),
        string(uv_strerror(uverr))
    );
}

template <class T, class X>
inline T get_handle (X* uvhp) {
    return static_cast<T>(reinterpret_cast<BackendHandle*>(uvhp->data));
}

}}}}
