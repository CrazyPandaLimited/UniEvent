#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/backend/BackendPoll.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPoll : UVHandle<BackendPoll> {
    UVPoll (uv_loop_t* loop, IPollListener* lst, sock_t sock) : UVHandle<BackendPoll>(lst) {
        int err = uv_poll_init_socket(loop, &uvh, sock);
        if (err) throw uvx_code_error(err);
        _init(&uvh);
    }

    UVPoll (uv_loop_t* loop, IPollListener* lst, int fd, std::nullptr_t) : UVHandle<BackendPoll>(lst) {
        int err = uv_poll_init(loop, &uvh, fd);
        if (err) throw uvx_code_error(err);
        _init(&uvh);
    }

    void start (int events) override {
        int err = uv_poll_start(&uvh, events, [](uv_poll_t* p, int status, int events) {
            get_handle<UVPoll*>(p)->handle_poll(events, uvx_status2err(status));
        });
        if (err) throw uvx_code_error(err);
    }

    void stop () override {
        int err = uv_poll_stop(&uvh);
        if (err) throw uvx_code_error(err);
    }

private:
    uv_poll_t uvh;
};

}}}}
