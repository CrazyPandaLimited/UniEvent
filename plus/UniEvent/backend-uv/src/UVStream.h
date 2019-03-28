#pragma once
#include "inc.h"
#include "UVHandle.h"
#include "UVRequest.h"
#include <panda/unievent/backend/BackendStream.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

template <class T>
struct UVStream : UVHandle<T> {
    UVStream (IStreamListener* lst) : UVHandle<T>(lst) {
    }

    bool readable () const noexcept override { return uv_is_readable(uvsp()); }
    bool writable () const noexcept override { return uv_is_writable(uvsp()); }

    void listen (int backlog) override {
        uvx_strict(uv_listen(uvsp(), backlog, [](uv_stream_t* p, int status) {
            get_handle<UVStream*>(p)->handle_connection(uvx_status2err(status));
        }));
    }

    CodeError accept (BackendStream* _client) {
        auto client = static_cast<UVStream*>(_client);
        return uvx_ce(uv_accept(uvsp(), client->uvsp()));
    }

private:
    uv_stream_t* uvsp () const { return reinterpret_cast<uv_stream_t*>(this->template uvhp); }

    static void _buf_alloc (uv_handle_t* p, size_t size, uv_buf_t* uvbuf) {
        auto buf = get_handle<UVStream*>(p)->buf_alloc(size);
        uvx_buf_alloc(buf, uvbuf);
    }

};

}}}}
