#pragma once
#include "Handle.h"
#include "Request.h"
#include <panda/string_view.h>
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent {

using std::string_view;
using panda::net::SockAddr;

struct UDP : virtual Handle, AllocatedObject<UDP> {
    using receive_fptr = void(UDP* handle, string& buf, const SockAddr& addr, unsigned flags, const CodeError* err);
    using receive_fn = function<receive_fptr>;

    using send_fptr = SendRequest::send_fptr;
    using send_fn = SendRequest::send_fn;

    CallbackDispatcher<receive_fptr> receive_event;
    CallbackDispatcher<send_fptr>    send_event;

    enum membership_t {
        LEAVE_GROUP = UV_LEAVE_GROUP,
        JOIN_GROUP  = UV_JOIN_GROUP
    };

    enum udp_flags {
        UDP_IPV6ONLY = UV_UDP_IPV6ONLY,
        UDP_PARTIAL  = UV_UDP_PARTIAL
    };

    UDP (Loop* loop = Loop::default_loop()) {
        int err = uv_udp_init(_pex_(loop), &uvh);
        if (err) throw CodeError(err);
        _init(&uvh);
    }
    
    UDP (Loop* loop, unsigned int flags) {
        int err = uv_udp_init_ex(_pex_(loop), &uvh, flags);
        if (err) throw CodeError(err);
        _init(&uvh);
    }

    virtual void open       (sock_t socket);
    virtual void bind       (const SockAddr&, unsigned flags = 0);
    virtual void bind       (string_view host, string_view service, const addrinfo* hints = &defhints, unsigned flags = 0);
    virtual void recv_start (receive_fn callback = nullptr);
    virtual void recv_stop  ();
    virtual void reset      () override;
    virtual void send       (SendRequest* req, const SockAddr& sa);

    void send (const string& data, const SockAddr& sa, send_fn callback = nullptr) {
        send(new SendRequest(data, callback), sa);
    }

    template <class It>
    void send (It begin, It end, const SockAddr& sa, send_fn callback = nullptr) {
        send(new SendRequest(begin, end, callback), sa);
    }

    SockAddr get_sockaddr () const {
        SockAddr ret;
        int sz = sizeof(ret);
        int err = uv_udp_getsockname(&uvh, ret.get(), &sz);
        if (err) throw CodeError(err);
        return ret;
    }

    void udp_membership (string_view multicast_addr, string_view interface_addr, membership_t membership) {
        PEXS_NULL_TERMINATE(multicast_addr, multicast_addr_cstr);
        PEXS_NULL_TERMINATE(interface_addr, interface_addr_cstr);
        int err = uv_udp_set_membership(&uvh, multicast_addr_cstr, interface_addr_cstr, static_cast<uv_membership>(membership));
        if (err) throw CodeError(err);
    }

    void udp_multicast_loop (bool on) {
        int err = uv_udp_set_multicast_loop(&uvh, on);
        if (err) throw CodeError(err);
    }

    void udp_multicast_ttl (int ttl) {
        int err = uv_udp_set_multicast_ttl(&uvh, ttl);
        if (err) throw CodeError(err);
    }

    void udp_broadcast (bool on) {
        int err = uv_udp_set_broadcast(&uvh, on);
        if (err) throw CodeError(err);
    }

    void udp_ttl (int ttl) {
        int err = uv_udp_set_ttl(&uvh, ttl);
        if (err) throw CodeError(err);
    }

    void call_on_receive (string& buf, const SockAddr& sa, unsigned flags, const CodeError* err) {
        on_receive(buf, sa, flags, err);
    }

    void call_on_send (const CodeError* err, SendRequest* req) {
        on_send(err, req);
        req->release();
        release();
    }
    
    using Handle::set_recv_buffer_size;
    using Handle::set_send_buffer_size;

protected:
    static const int UF_LAST = HF_LAST;
    
    void on_handle_reinit () override;

    virtual void on_receive (string& buf, const SockAddr& sa, unsigned flags, const CodeError* err);
    virtual void on_send    (const CodeError* err, SendRequest* req);

private:
    uv_udp_t uvh;
    static addrinfo defhints;

    static void uvx_on_receive (uv_udp_t* handle, ssize_t nread, const uv_buf_t* uvbuf, const sockaddr* addr, unsigned flags);
    static void uvx_on_send    (uv_udp_send_t* uvreq, int status);
};

}}
