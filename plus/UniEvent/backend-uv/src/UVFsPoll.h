#pragma once
#include "UVHandle.h"
#include <panda/unievent/backend/BackendFsPoll.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVFsPoll : UVHandle<BackendFsPoll, uv_fs_poll_t> {
    UVFsPoll (UVLoop* loop, IFsPollListener* lst) : UVHandle<BackendFsPoll, uv_fs_poll_t>(loop, lst) {
        uvx_strict(uv_fs_poll_init(loop->uvloop, &uvh));
    }

    void start (std::string_view path, unsigned int interval) override {
        UE_NULL_TERMINATE(path, path_str);
        uvx_strict(uv_fs_poll_start(&uvh, on_fs_poll, path_str, interval));
    }

    void stop () override {
        uvx_strict(uv_fs_poll_stop(&uvh));
    }

    panda::string path () const override {
        size_t len = 0;
        auto err = uv_fs_poll_getpath(const_cast<uv_fs_poll_t*>(&uvh), nullptr, &len); // libuv returns len counting null-byte
        if (err && err != UV_ENOBUFS) throw uvx_code_error(err);

        panda::string str(len);
        uvx_strict(uv_fs_poll_getpath(const_cast<uv_fs_poll_t*>(&uvh), str.buf(), &len));
        str.length(len);

        return str;
    }

private:
    static void on_fs_poll (uv_fs_poll_t* p, int status, const uv_stat_t* _prev, const uv_stat_t* _cur) {
        Stat prev, cur;
        if (!status) {
            uvx_stat2ue(_prev, prev);
            uvx_stat2ue(_cur, cur);
        }
        get_handle<UVFsPoll*>(p)->handle_fs_poll(prev, cur, uvx_ce(status));
    }
};

}}}}
