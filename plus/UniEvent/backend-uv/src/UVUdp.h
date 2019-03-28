#pragma once
#include "inc.h"
#include "UVHandle.h"
#include "UVRequest.h"
#include <panda/unievent/backend/BackendUdp.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVSendRequest : UVRequest<BackendSendRequest>, panda::lib::AllocatedObject<UVSendRequest> {
    UVSendRequest (ISendListener* l) : UVRequest<BackendSendRequest>(l) {
        _init(&uvr);
    }

    BackendHandle* handle () const noexcept override {
        return get_handle(uvr.handle);
    }

    void destroy () noexcept override {
        if (active) uvr.cb = [](uv_udp_send_t* p, int) { delete get_request<UVSendRequest*>(p); }; // cant make uv request stop, so remove as it completes
        else delete this;
    }

    uv_udp_send_t uvr;
};

struct UVUdp : UVHandle<BackendUdp> {
    UVUdp (uv_loop_t* loop, IUdpListener* lst, int domain) : UVHandle<BackendUdp>(lst) {
        uvx_strict(domain == AF_UNSPEC ? uv_udp_init(loop, &uvh) : uv_udp_init_ex(loop, &uvh, domain));
        _init(&uvh);
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

    net::SockAddr get_sockaddr () override {
        net::SockAddr ret;
        int sz = sizeof(ret);
        uvx_strict(uv_udp_getsockname(&uvh, ret.get(), &sz));
        return ret;
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

    CodeError send (const std::vector<string>& bufs, const net::SockAddr& addr, BackendSendRequest* req) override {
        auto nbufs = bufs.size();
        uv_buf_t uvbufs[nbufs];
        uv_buf_t* ptr = uvbufs;
        for (const auto& str : bufs) {
            ptr->base = (char*)str.data();
            ptr->len  = str.length();
            ++ptr;
        }

        auto uvreq = static_cast<UVSendRequest*>(req);
        auto err = uv_udp_send(&uvreq->uvr, &uvh, uvbufs, nbufs, addr.get(), _on_send);
        if (!err) uvreq->active = true;
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
    uv_udp_t uvh;

    static void _on_send (uv_udp_send_t* p, int status) {
        auto req = get_request<UVSendRequest*>(p);
        req->active = false;
        req->handle_send(uvx_status2err(status));
    }

    static void _buf_alloc (uv_handle_t* p, size_t size, uv_buf_t* uvbuf) {
        auto buf = get_handle<UVUdp*>(p)->buf_alloc(size);
        uvx_buf_alloc(buf, uvbuf);
    }

    static void _on_receive (uv_udp_t* p, ssize_t nread, const uv_buf_t* uvbuf, const sockaddr* addr, unsigned flags) {
        auto buf = uvx_detach_buf(uvbuf);

        if (!nread && !addr) return; // nothing to read

        int status = 0;
        if (nread < 0) {
            status = nread;
            nread = 0;
        }

        buf.length(nread); // set real buf len

        get_handle<UVUdp*>(p)->handle_receive(buf, addr, flags, uvx_status2err(status));
    }
};

}}}}
