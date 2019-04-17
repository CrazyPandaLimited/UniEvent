#pragma once
#include "UVStream.h"
#include <panda/unievent/backend/BackendTcp.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVTcp : UVStream<BackendTcp, uv_tcp_t> {
    UVTcp (UVLoop* loop, IStreamListener* lst, int domain) : UVStream<BackendTcp, uv_tcp_t>(loop, lst) {
        if (domain == AF_UNSPEC) uvx_strict(uv_tcp_init(loop->uvloop, &uvh));
        else                     uvx_strict(uv_tcp_init_ex(loop->uvloop, &uvh, domain));
    }

    void open (sock_t sock) override {
        uvx_strict(uv_tcp_open(&uvh, sock));
    }

    void bind (const net::SockAddr& addr, unsigned flags) override {
        unsigned uv_flags = 0;
        if (flags & Flags::IPV6ONLY) uv_flags |= UV_TCP_IPV6ONLY;
        uvx_strict(uv_tcp_bind(&uvh, addr.get(), uv_flags));
    }

//    virtual CodeError connect (std::string_view name, BackendConnectRequest* _req) override {
//        UE_NULL_TERMINATE(name, name_str);
//        auto req = static_cast<UVConnectRequest*>(_req);
//        uv_pipe_connect(&req->uvr, &uvh, name_str, on_connect);
//        req->active = true;
//        return {};
//    }
//
//    optional<string> sockname () const override { return uvx_sockname(&uvh, &uv_pipe_getsockname); }
//    optional<string> peername () const override { return uvx_sockname(&uvh, &uv_pipe_getpeername); }
//
//    void pending_instances (int count) override {
//        uv_pipe_pending_instances(&uvh, count);
//    }
};

}}}}
