#pragma once
#include "BackendStream.h"

namespace panda { namespace unievent { namespace backend {

struct BackendTcp : BackendStream {
    struct Flags {
        static const constexpr int IPV6ONLY = 1;
    };

    BackendTcp (BackendLoop* loop, IStreamListener* lst) : BackendStream(loop, lst) {}

    virtual void open (sock_t) = 0;
    virtual void bind (const net::SockAddr&, unsigned flags) = 0;

    virtual CodeError connect (const net::SockAddr&, BackendConnectRequest*) = 0;

    virtual net::SockAddr sockaddr () const = 0;
    virtual net::SockAddr peeraddr () const = 0;

    virtual void set_nodelay              (bool enable) = 0;
    virtual void set_keepalive            (bool enable, unsigned delay) = 0;
    virtual void set_simultaneous_accepts (bool enable) = 0;
};

}}}
