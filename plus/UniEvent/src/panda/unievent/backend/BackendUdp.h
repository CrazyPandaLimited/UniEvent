#pragma once
#include "BackendHandle.h"
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent { namespace backend {

struct IUdpListener {
    virtual string buf_alloc      (size_t cap) = 0;
    virtual void   handle_receive (string& buf, const net::SockAddr& addr, unsigned flags, const CodeError& err) = 0;
};

struct BackendSendRequest : BackendRequest { using BackendRequest::BackendRequest; };

struct BackendUdp : BackendHandle {
    struct Flags {
        static const constexpr int PARTIAL   = 1;
        static const constexpr int IPV6ONLY  = 2;
        static const constexpr int REUSEADDR = 4;
    };

    enum class Membership {
        LEAVE_GROUP = 0,
        JOIN_GROUP
    };

    BackendUdp (BackendLoop* loop, IUdpListener* lst) : BackendHandle(loop), listener(lst) {}

    virtual BackendRequest* new_send_request (IRequestListener*) = 0;

    string buf_alloc (size_t size) noexcept { return BackendHandle::buf_alloc(size, listener); }

    virtual void      open       (sock_t sock) = 0;
    virtual void      bind       (const net::SockAddr&, unsigned flags) = 0;
    virtual void      connect    (const net::SockAddr&) = 0;
    virtual void      recv_start () = 0;
    virtual void      recv_stop  () = 0;
    virtual CodeError send       (const std::vector<string>& bufs, const net::SockAddr& addr, BackendSendRequest*) = 0;

    virtual net::SockAddr sockaddr () = 0;
    virtual net::SockAddr peeraddr () = 0;

    virtual optional<fh_t> fileno () const = 0;

    virtual int  recv_buffer_size () const    = 0;
    virtual void recv_buffer_size (int value) = 0;
    virtual int  send_buffer_size () const    = 0;
    virtual void send_buffer_size (int value) = 0;

    virtual void set_membership          (std::string_view multicast_addr, std::string_view interface_addr, Membership m) = 0;
    virtual void set_multicast_loop      (bool on) = 0;
    virtual void set_multicast_ttl       (int ttl) = 0;
    virtual void set_multicast_interface (std::string_view interface_addr) = 0;
    virtual void set_broadcast           (bool on) = 0;
    virtual void set_ttl                 (int ttl) = 0;

    void handle_receive (string& buf, const net::SockAddr& addr, unsigned flags, const CodeError& err) {
        ltry([&]{ listener->handle_receive(buf, addr, flags, err); });
    }

    IUdpListener* listener;
};

}}}
