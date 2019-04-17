#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendPoll.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPoll : UVHandle<BackendPoll, uv_poll_t> {
    UVPoll (UVLoop* loop, IPollListener* lst, sock_t sock) : UVHandle<BackendPoll, uv_poll_t>(loop, lst) {
        uvx_strict(uv_poll_init_socket(loop->uvloop, &uvh, sock));
    }

    UVPoll (UVLoop* loop, IPollListener* lst, int fd, std::nullptr_t) : UVHandle<BackendPoll, uv_poll_t>(loop, lst) {
        uvx_strict(uv_poll_init(loop->uvloop, &uvh, fd));
    }

    void start (int events) override {
        uvx_strict(uv_poll_start(&uvh, events, [](uv_poll_t* p, int status, int events) {
            get_handle<UVPoll*>(p)->handle_poll(events, uvx_ce(status));
        }));
    }

    void stop () override {
        uvx_strict(uv_poll_stop(&uvh));
    }

    optional<fd_t> fileno () const override { return uvx_fileno(uvhp()); }
};

}}}}
