#pragma once
#include "inc.h"
#include "UVHandle.h"
#include "UVRequest.h"
#include <panda/unievent/backend/BackendStream.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVConnectRequest : UVRequest<BackendConnectRequest, uv_connect_t>, AllocatedObject<UVConnectRequest> {
    UVConnectRequest (IConnectListener* l) : UVRequest<BackendConnectRequest, uv_connect_t>(l) {}
};

struct UVShutdownRequest : UVRequest<BackendShutdownRequest, uv_shutdown_t>, AllocatedObject<UVShutdownRequest> {
    UVShutdownRequest (IShutdownListener* l) : UVRequest<BackendShutdownRequest, uv_shutdown_t>(l) {}
};

template <class Base, class UvReq>
struct UVStream : UVHandle<Base, UvReq> {
    UVStream (IStreamListener* lst) : UVHandle<Base, UvReq>(lst) {}

    bool readable () const noexcept override { return uv_is_readable(uvsp()); }
    bool writable () const noexcept override { return uv_is_writable(uvsp()); }

    void listen (int backlog) override {
        uvx_strict(uv_listen(uvsp(), backlog, on_connection));
    }

    CodeError accept (BackendStream* _client) {
        auto client = static_cast<UVStream*>(_client);
        return uvx_ce(uv_accept(uvsp(), client->uvsp()));
    }

    CodeError read_start () override {
        return uvx_ce(uv_read_start(uvsp(), _buf_alloc, on_read));
    }

    void read_stop () override {
        uv_read_stop(uvsp());
    }

    CodeError shutdown (BackendShutdownRequest* _req) {
        auto req = static_cast<UVShutdownRequest*>(_req);
        int err = uv_shutdown(&req->uvr, uvsp(), on_shutdown);
        if (!err) req->active = true;
        return uvx_ce(err);
    }

protected:
    static void on_connect (uv_connect_t* p, int status) {
        auto req = get_request<UVConnectRequest*>(p);
        req->active = false;
        req->handle_connect(uvx_ce(status));
    }

private:
    uv_stream_t* uvsp () const { return (uv_stream_t*)&this->template uvh; }

    static void on_connection (uv_stream_t* p, int status) {
        get_handle<UVStream*>(p)->handle_connection(uvx_ce(status));
    }

    static void _buf_alloc (uv_handle_t* p, size_t size, uv_buf_t* uvbuf) {
        auto buf = get_handle<UVStream*>(p)->buf_alloc(size);
        uvx_buf_alloc(buf, uvbuf);
    }

    static void on_read (uv_stream_t* p, ssize_t nread, const uv_buf_t* uvbuf) {
        auto h   = get_handle<UVStream*>(p);
        auto buf = uvx_detach_buf(uvbuf);

        ssize_t err = 0;
        if      (nread < 0) std::swap(err, nread);
        else if (nread == 0) return; // UV just wants to release the buf

        if (err == UV_EOF) {
            h->handle_eof();
            return;
        }

        buf.length(nread); // set real buf len
        h->handle_read(buf, uvx_ce(err));
    }

    static void on_shutdown (uv_shutdown_t* p, int status) {
        auto req = get_request<UVShutdownRequest*>(p);
        req->active = false;
        req->handle_shutdown(uvx_ce(status));
    }
};

}}}}
