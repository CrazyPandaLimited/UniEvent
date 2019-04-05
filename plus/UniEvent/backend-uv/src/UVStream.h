#pragma once
#include "inc.h"
#include "UVHandle.h"
#include "UVRequest.h"
#include <panda/unievent/backend/BackendStream.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVConnectRequest : UVRequest<BackendConnectRequest, uv_connect_t>, AllocatedObject<UVConnectRequest> {
    UVConnectRequest (IConnectListener* l) : UVRequest<BackendConnectRequest, uv_connect_t>(l) {}
};

template <class Base, class UvReq>
struct UVStream : UVHandle<Base, UvReq> {
    UVStream (IStreamListener* lst) : UVHandle<Base, UvReq>(lst) {}

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

protected:
    static void on_connect (uv_connect_t* p, int status) {
        auto req = get_request<UVConnectRequest*>(p);
        req->active = false;
        req->handle_connect(uvx_status2err(status));
    }

private:
    uv_stream_t* uvsp () const { return (uv_stream_t*)&this->template uvh; }

    static void _buf_alloc (uv_handle_t* p, size_t size, uv_buf_t* uvbuf) {
        auto buf = get_handle<UVStream*>(p)->buf_alloc(size);
        uvx_buf_alloc(buf, uvbuf);
    }
};

}}}}
