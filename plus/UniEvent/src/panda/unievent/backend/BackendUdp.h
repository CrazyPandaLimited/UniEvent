#pragma once
#include "BackendHandle.h"
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent { namespace backend {

struct IUdpListener {
    virtual string buf_alloc      (size_t cap) = 0;
    virtual void   handle_receive (string& buf, const net::SockAddr& addr, unsigned flags, const CodeError* err) = 0;
};

struct ISendListener {
    virtual void handle_send (const CodeError*) = 0;
};

struct BackendSendRequest : BackendRequest {
    BackendSendRequest (ISendListener* l) : listener(l) {}

    void handle_send (const CodeError* err) noexcept {
        ltry([&]{ listener->handle_send(err); });
    }

    ISendListener* listener;
};

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

    BackendUdp (IUdpListener* l) : listener(l) {}

    virtual void open (sock_t sock) = 0;
    virtual void bind (const net::SockAddr& addr, unsigned flags) = 0;

    virtual net::SockAddr get_sockaddr () = 0;

    virtual void set_membership          (std::string_view multicast_addr, std::string_view interface_addr, Membership m) = 0;
    virtual void set_multicast_loop      (bool on) = 0;
    virtual void set_multicast_ttl       (int ttl) = 0;
    virtual void set_multicast_interface (std::string_view interface_addr) = 0;
    virtual void set_broadcast           (bool on) = 0;
    virtual void set_ttl                 (int ttl) = 0;

    virtual CodeError send (const std::vector<string>& bufs, const net::SockAddr& addr, BackendSendRequest* req) = 0;

    virtual void recv_start () = 0;
    virtual void recv_stop  () = 0;

    virtual BackendSendRequest* new_send_request (ISendListener*) = 0;

    string buf_alloc (size_t size) noexcept {
        if (size < MIN_ALLOC_SIZE) size = MIN_ALLOC_SIZE;
        try {
            return listener->buf_alloc(size);
        }
        catch (...) {
            return {};
        }
    }

    void handle_receive (string& buf, const net::SockAddr& addr, unsigned flags, const CodeError* err) {
        ltry([&]{ listener->handle_receive(buf, addr, flags, err); });
    }

    IUdpListener* listener;
};

}}}
