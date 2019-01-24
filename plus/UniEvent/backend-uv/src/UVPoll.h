#pragma once
#include "inc.h"
#include "UVHandle.h"
#include <panda/unievent/Poll.h>
#include <panda/unievent/backend/BackendPoll.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPoll : UVHandle<BackendPoll> {
    UVPoll (uv_loop_t* loop, Poll* frontend, sock_t sock) : UVHandle<BackendPoll>(frontend) {
        int err = uv_poll_init_socket(loop, &uvh, sock);
        if (err) throw uvx_code_error(err);
        _init(&uvh);
    }

    UVPoll (uv_loop_t* loop, Poll* frontend, int fd, std::nullptr_t) : UVHandle<BackendPoll>(frontend) {
        int err = uv_poll_init(loop, &uvh, fd);
        if (err) throw uvx_code_error(err);
        _init(&uvh);
    }

    void start (int events) override {
        int err = uv_poll_start(&uvh, events, _call);
        if (err) throw uvx_code_error(err);
    }

    void stop () override {
        int err = uv_poll_stop(&uvh);
        if (err) throw uvx_code_error(err);
    }

private:
    uv_poll_t uvh;

    static void _call (uv_poll_t* p, int status, int events) {
        auto h = get_handle<UVPoll*>(p);
        if (h->frontend) h->frontend->call_now(events, uvx_status2err(status));
    }
};

}}}}
