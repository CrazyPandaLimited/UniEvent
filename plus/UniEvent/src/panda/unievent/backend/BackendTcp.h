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

//    virtual CodeError connect (std::string_view name, BackendConnectRequest* req) = 0;
//
//    virtual optional<string> sockname () const = 0;
//    virtual optional<string> peername () const = 0;
//
//    virtual void pending_instances (int count) = 0;
};

}}}
