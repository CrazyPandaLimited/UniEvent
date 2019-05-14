#pragma once
#include <uv.h>
#include <panda/unievent/util.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/Error.h>
#include <panda/unievent/backend/BackendHandle.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

using panda::lib::AllocatedObject;

static inline void      uvx_strict (int err) { if (err) throw uvx_code_error(err); }
static inline CodeError uvx_ce     (int err) { return err ? uvx_code_error(err) : CodeError(); }

template <class T = BackendHandle*, class X>
static inline T get_handle (X* uvhp) {
    return static_cast<T>(reinterpret_cast<BackendHandle*>(uvhp->data));
}

template <class T = BackendRequest*, class X>
static inline T get_request (X* uvrp) {
    return static_cast<T>(reinterpret_cast<BackendRequest*>(uvrp->data));
}

static inline void uvx_buf_alloc (string& buf, uv_buf_t* uvbuf) {
    char* ptr = buf.shared_buf();
    auto  cap = buf.shared_capacity();

    size_t align = (size_t(ptr) + cap) % sizeof(void*);
    cap -= align;

    auto availcap = cap - sizeof(string);

    new ((string*)(ptr + availcap)) string(buf); // save string object at the end of the buffer, keeping it's internal ptr alive

    uvbuf->base = ptr;
    uvbuf->len  = availcap;
}

static inline string uvx_detach_buf (const uv_buf_t* uvbuf) {
    if (!uvbuf->base) return {}; // in some cases of eof there may be no buffer
    auto buf_ptr = (string*)(uvbuf->base + uvbuf->len);
    string ret = *buf_ptr;
    buf_ptr->~string();
    return ret;
}

#define UVX_FILL_BUFS(bufs, uvbufs)     \
    uv_buf_t uvbufs[bufs.size()];       \
    uv_buf_t* ptr = uvbufs;             \
    for (const auto& str : bufs) {      \
        ptr->base = (char*)str.data();  \
        ptr->len  = str.length();       \
        ++ptr;                          \
    }

template <class Handle, class Func>
static inline net::SockAddr uvx_sockaddr (Handle uvhp, Func&& f) {
    net::SockAddr ret;
    int sz = sizeof(ret);
    int err = f(uvhp, ret.get(), &sz);
    if (err) {
        if (err == UV_ENOTCONN || err == UV_EBADF || err == UV_EINVAL) return {};
        throw uvx_code_error(err);
    }
    return ret;
}

static inline optional<fd_t> uvx_fileno (const uv_handle_t* p) {
    uv_os_fd_t fd; //should be compatible type
    int err = uv_fileno(p, &fd);
    if (!err) return {fd};
    if (err == UV_EBADF) return {};
    throw uvx_code_error(err);
}

static inline int uvx_recv_buffer_size (const uv_handle_t* p) {
    int ret = 0;
    uvx_strict(uv_recv_buffer_size((uv_handle_t*)p, &ret));
    return ret;
}

static inline void uvx_recv_buffer_size (uv_handle_t* p, int value) {
    uvx_strict(uv_recv_buffer_size(p, &value));
}

static inline int uvx_send_buffer_size (const uv_handle_t* p) {
    int ret = 0;
    uvx_strict(uv_send_buffer_size((uv_handle_t*)p, &ret));
    return ret;
}

static inline void uvx_send_buffer_size (uv_handle_t* p, int value) {
    uvx_strict(uv_send_buffer_size(p, &value));
}

static inline void uvx_tv2ue (const uv_timespec_t& from, TimeVal& to) {
    to.sec  = from.tv_sec;
    to.usec = from.tv_nsec;
}

static inline void uvx_stat2ue (const uv_stat_t* from, Stat& to) {
    to.dev     = from->st_dev;
    to.mode    = from->st_mode;
    to.nlink   = from->st_nlink;
    to.uid     = from->st_uid;
    to.gid     = from->st_gid;
    to.rdev    = from->st_rdev;
    to.ino     = from->st_ino;
    to.size    = from->st_size;
    to.blksize = from->st_blksize;
    to.blocks  = from->st_blocks;
    to.flags   = from->st_flags;
    to.gen     = from->st_gen;
    uvx_tv2ue(from->st_atim, to.atime);
    uvx_tv2ue(from->st_mtim, to.mtime);
    uvx_tv2ue(from->st_ctim, to.ctime);
    uvx_tv2ue(from->st_birthtim, to.birthtime);
}

}}}}
