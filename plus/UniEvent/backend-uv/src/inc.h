#pragma once
#include <uv.h>
#include <panda/unievent/util.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/Error.h>
#include <panda/unievent/backend/BackendHandle.h>

#define uvx_status2err(status) (status ? static_cast<const CodeError*>(uvx_code_error(status)) : nullptr)

namespace panda { namespace unievent { namespace backend { namespace uv {

using panda::lib::AllocatedObject;

inline void      uvx_strict (int err) { if (err) throw uvx_code_error(err); }
inline CodeError uvx_ce     (int err) { return err ? uvx_code_error(err) : CodeError(); }

template <class T = BackendHandle*, class X>
inline T get_handle (X* uvhp) {
    return static_cast<T>(reinterpret_cast<BackendHandle*>(uvhp->data));
}

template <class T = BackendRequest*, class X>
inline T get_request (X* uvrp) {
    return static_cast<T>(reinterpret_cast<BackendRequest*>(uvrp->data));
}

inline void uvx_buf_alloc (string& buf, uv_buf_t* uvbuf) {
    char* ptr = buf.shared_buf();
    auto  cap = buf.shared_capacity();

    size_t align = (size_t(ptr) + cap) % sizeof(void*);
    cap -= align;

    auto availcap = cap - sizeof(string);

    new ((string*)(ptr + availcap)) string(buf); // save string object at the end of the buffer, keeping it's internal ptr alive

    uvbuf->base = ptr;
    uvbuf->len  = availcap;
}

inline string uvx_detach_buf (const uv_buf_t* uvbuf) {
    auto buf_ptr = (string*)(uvbuf->base + uvbuf->len);
    string ret = *buf_ptr;
    buf_ptr->~string();
    return ret;
}

template <class Handle, class Func>
inline net::SockAddr uvx_sockaddr (Handle uvhp, Func&& f) {
    net::SockAddr ret;
    int sz = sizeof(ret);
    int err = f(uvhp, ret.get(), &sz);
    if (err) {
        if (err == UV_ENOTCONN) return {};
        throw uvx_code_error(err);
    }
    return ret;
}

}}}}
