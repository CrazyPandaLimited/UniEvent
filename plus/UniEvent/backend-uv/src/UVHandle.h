#pragma once
#include "inc.h"
#include <panda/unievent/backend/BackendHandle.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

template <class Base>
struct UVHandle : Base {
protected:
    using Base::Base;

    void _init (void* p) {
        uvhp = static_cast<uv_handle_t*>(p);
        uvhp->data = static_cast<BackendHandle*>(this);
    }

    BackendLoop* loop () const noexcept override {
        return reinterpret_cast<BackendLoop*>(uvhp->loop->data);
    }

    bool active () const override {
        return uv_is_active(uvhp);
    }

    void set_weak () override {
        uv_unref(uvhp);
    }

    void unset_weak () override {
        uv_ref(uvhp);
    }

    optional<fd_t> fileno () const override {
        uv_os_fd_t fd; //should be compatible type
        int err = uv_fileno(uvhp, &fd);
        if (!err) return {fd};
        if (err == UV_EBADF) return {};
        throw uvx_code_error(err);
    }

    int recv_buffer_size () const override {
        int ret = 0;
        uvx_strict(uv_recv_buffer_size(uvhp, &ret));
        return ret;
    }

    void recv_buffer_size (int value) override {
        uvx_strict(uv_recv_buffer_size(uvhp, &value));
    }

    int send_buffer_size () const override {
        int ret = 0;
        uvx_strict(uv_send_buffer_size(uvhp, &ret));
        return ret;
    }

    void send_buffer_size (int value) override {
        uvx_strict(uv_send_buffer_size(uvhp, &value));
    }

    void destroy () noexcept override {
        _EDEBUGTHIS("%s", uvx_type_name(uvhp));
        this->template listener = nullptr;
        uv_close(uvhp, uvx_on_close);
    }

    uv_handle_t* uvhp;

private:
    static void uvx_on_close (uv_handle_t* p) {
        auto h = get_handle(p);
        _EDEBUG("[%p] %s", h, uvx_type_name(p));
        delete h;
    }

    static const char* uvx_type_name (uv_handle_t* p) {
#       define XX(uc,lc) case UV_##uc : return #lc;
        switch (p->type) {
            UV_HANDLE_TYPE_MAP(XX)
            case UV_FILE: return "file";
            default: break;
        }
#       undef XX
        return "";
    }
};

}}}}
