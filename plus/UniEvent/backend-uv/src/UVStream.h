#pragma once
#include "UVHandle.h"
#include "UVRequest.h"
#include <panda/unievent/backend/StreamImpl.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVConnectRequest : UVRequest<ConnectRequestImpl, uv_connect_t>, AllocatedObject<UVConnectRequest> {
    using UVRequest<ConnectRequestImpl, uv_connect_t>::UVRequest;
};

struct UVWriteRequest : UVRequest<WriteRequestImpl, uv_write_t>, AllocatedObject<UVWriteRequest> {
    using UVRequest<WriteRequestImpl, uv_write_t>::UVRequest;
};

struct UVShutdownRequest : UVRequest<ShutdownRequestImpl, uv_shutdown_t>, AllocatedObject<UVShutdownRequest> {
    using UVRequest<ShutdownRequestImpl, uv_shutdown_t>::UVRequest;
};

template <class Base, class UvReq>
struct UVStream : UVHandle<Base, UvReq> {
    UVStream (UVLoop* loop, IStreamListener* lst) : UVHandle<Base, UvReq>(loop, lst) {}

    ConnectRequestImpl*  new_connect_request  (IRequestListener* l) override { return new UVConnectRequest(this, l); }
    WriteRequestImpl*    new_write_request    (IRequestListener* l) override { return new UVWriteRequest(this, l); }
    ShutdownRequestImpl* new_shutdown_request (IRequestListener* l) override { return new UVShutdownRequest(this, l); }

    void listen (int backlog) override {
        uvx_strict(uv_listen(uvsp(), backlog, on_connection));
    }

    CodeError accept (StreamImpl* _client) override {
        auto client = static_cast<UVStream*>(_client);
        return uvx_ce(uv_accept(uvsp(), client->uvsp()));
    }

    CodeError read_start () override {
        return uvx_ce(uv_read_start(uvsp(), _buf_alloc, on_read));
    }

    void read_stop () override {
        uv_read_stop(uvsp());
    }

    CodeError write (const std::vector<string>& bufs, WriteRequestImpl* _req) override {
        auto req = static_cast<UVWriteRequest*>(_req);
        UVX_FILL_BUFS(bufs, uvbufs);
        auto err = uv_write(&req->uvr, uvsp(), uvbufs, bufs.size(), on_write);
        if (err) return uvx_code_error(err);
        req->active = true;
        if (!uvsp()->write_queue_size) req->handle_event({}); // written synchronously
        return {};
    }

    void shutdown (ShutdownRequestImpl* _req) override {
        auto req = static_cast<UVShutdownRequest*>(_req);
        int err = uv_shutdown(&req->uvr, uvsp(), on_shutdown);
        if (err) return req->handle_event(uvx_code_error(err));
        req->active = true;
    }

    optional<fh_t> fileno () const override { return uvx_fileno(this->template uvhp()); }

    bool   readable         () const noexcept override { return uv_is_readable(uvsp()); }
    bool   writable         () const noexcept override { return uv_is_writable(uvsp()); }
    size_t write_queue_size () const noexcept override { return uvsp()->write_queue_size; }

    int  recv_buffer_size ()    const override { return uvx_recv_buffer_size(this->template uvhp()); }
    void recv_buffer_size (int value) override { uvx_recv_buffer_size(this->template uvhp(), value); }
    int  send_buffer_size ()    const override { return uvx_send_buffer_size(this->template uvhp()); }
    void send_buffer_size (int value) override { uvx_send_buffer_size(this->template uvhp(), value); }

protected:
    static void on_connect (uv_connect_t* p, int status) {
        auto req = get_request<UVConnectRequest*>(p);
        req->active = false;
        req->handle_event(uvx_ce(status));
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
        _EDEBUG("%p\n", h);

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

    static void on_write (uv_write_t* p, int status) {
        auto req = get_request<UVWriteRequest*>(p);
        req->active = false;
        req->handle_event(uvx_ce(status));
    }

    static void on_shutdown (uv_shutdown_t* p, int status) {
        auto req = get_request<UVShutdownRequest*>(p);
        req->active = false;
        req->handle_event(uvx_ce(status));
    }
};

}}}}