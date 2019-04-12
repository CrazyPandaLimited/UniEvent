#pragma once
#include "inc.h"
#include "UVHandle.h"
#include "UVRequest.h"
#include <panda/unievent/backend/BackendUdp.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVSendRequest : UVRequest<BackendSendRequest, uv_udp_send_t>, AllocatedObject<UVSendRequest> {
    UVSendRequest (ISendListener* l) : UVRequest<BackendSendRequest, uv_udp_send_t>(l) {}
};

struct UVUdp : UVHandle<BackendUdp, uv_udp_t> {
    UVUdp (uv_loop_t* loop, IUdpListener* lst, int domain) : UVHandle<BackendUdp, uv_udp_t>(lst) {
        uvx_strict(domain == AF_UNSPEC ? uv_udp_init(loop, &uvh) : uv_udp_init_ex(loop, &uvh, domain));
    }

    void open (sock_t sock) override {
        uvx_strict(uv_udp_open(&uvh, sock));
    }

    void bind (const net::SockAddr& addr, unsigned flags) override {
        unsigned uv_flags = 0;
        if (flags & Flags::IPV6ONLY)  uv_flags |= UV_UDP_IPV6ONLY;
        if (flags & Flags::REUSEADDR) uv_flags |= UV_UDP_REUSEADDR;
        uvx_strict(uv_udp_bind(&uvh, addr.get(), uv_flags));
    }

    void connect (const net::SockAddr& addr) override {
        uvx_strict(uv_udp_connect(&uvh, addr ? addr.get() : nullptr));
    }

    net::SockAddr sockaddr () override {
        return uvx_sockaddr(&uvh, &uv_udp_getsockname);
    }

    net::SockAddr peeraddr () override {
        return uvx_sockaddr(&uvh, &uv_udp_getpeername);
    }

    void set_membership (std::string_view multicast_addr, std::string_view interface_addr, Membership membership) override {
        uv_membership uvmemb = uv_membership();
        switch (membership) {
            case Membership::LEAVE_GROUP : uvmemb = UV_LEAVE_GROUP; break;
            case Membership::JOIN_GROUP  : uvmemb = UV_JOIN_GROUP;  break;
        }
        UE_NULL_TERMINATE(multicast_addr, multicast_addr_cstr);
        UE_NULL_TERMINATE(interface_addr, interface_addr_cstr);
        uvx_strict(uv_udp_set_membership(&uvh, multicast_addr_cstr, interface_addr_cstr, uvmemb));
    }

    void set_multicast_loop (bool on) override {
        uvx_strict(uv_udp_set_multicast_loop(&uvh, on));
    }

    void set_multicast_ttl (int ttl) override {
        uvx_strict(uv_udp_set_multicast_ttl(&uvh, ttl));
    }

    void set_multicast_interface (std::string_view interface_addr) override {
        UE_NULL_TERMINATE(interface_addr, interface_addr_cstr);
        uvx_strict(uv_udp_set_multicast_interface(&uvh, interface_addr_cstr));
    }

    void set_broadcast (bool on) override {
        uvx_strict(uv_udp_set_broadcast(&uvh, on));
    }

    void set_ttl (int ttl) override {
        uvx_strict(uv_udp_set_ttl(&uvh, ttl));
    }

    CodeError send (const std::vector<string>& bufs, const net::SockAddr& addr, BackendSendRequest* _req) override {
        auto nbufs = bufs.size();
        uv_buf_t uvbufs[nbufs];
        uv_buf_t* ptr = uvbufs;
        for (const auto& str : bufs) {
            ptr->base = (char*)str.data();
            ptr->len  = str.length();
            ++ptr;
        }

        auto req = static_cast<UVSendRequest*>(_req);
        auto err = uv_udp_send(&req->uvr, &uvh, uvbufs, nbufs, addr.get(), _on_send);
        if (!err) req->active = true;
        return uvx_ce(err);
    }

    void recv_start () override {
        uvx_strict(uv_udp_recv_start(&uvh, _buf_alloc, _on_receive));
    }

    void recv_stop  () override {
        uvx_strict(uv_udp_recv_stop(&uvh));
    }

    BackendSendRequest* new_send_request (ISendListener* l) override { return new UVSendRequest(l); }

private:
    static void _on_send (uv_udp_send_t* p, int status) {
        auto req = get_request<UVSendRequest*>(p);
        req->active = false;
        req->handle_send(uvx_ce(status));
    }

    static void _buf_alloc (uv_handle_t* p, size_t size, uv_buf_t* uvbuf) {
        auto buf = get_handle<UVUdp*>(p)->buf_alloc(size);
        uvx_buf_alloc(buf, uvbuf);
    }

    static void _on_receive (uv_udp_t* p, ssize_t nread, const uv_buf_t* uvbuf, const struct sockaddr* addr, unsigned flags) {
        auto h   = get_handle<UVUdp*>(p);
        auto buf = uvx_detach_buf(uvbuf);

        if (!nread && !addr) return; // nothing to read

        ssize_t err = 0;
        if (nread < 0) std::swap(err, nread);
        buf.length(nread); // set real buf len

        h->handle_receive(buf, addr, flags, uvx_ce(err));
    }
};

}}}}
