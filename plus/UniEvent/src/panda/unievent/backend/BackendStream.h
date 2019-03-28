#pragma once
#include "BackendHandle.h"
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent { namespace backend {

struct IStreamListener {
    virtual string buf_alloc         (size_t cap) = 0;
    virtual void   handle_connection (const CodeError*) = 0;
};

//struct ISendListener {
//    virtual void handle_send (const CodeError*) = 0;
//};

//struct BackendSendRequest : BackendRequest {
//    BackendSendRequest (ISendListener* l) : listener(l) {}
//
//    void handle_send (const CodeError* err) noexcept {
//        ltry([&]{ listener->handle_send(err); });
//    }
//
//    ISendListener* listener;
//};

struct BackendStream : BackendHandle {
    BackendStream (IStreamListener* l) : listener(l) {}

    string buf_alloc (size_t size) noexcept { return BackendHandle::buf_alloc(size, listener); }

    virtual bool readable () const noexcept = 0;
    virtual bool writable () const noexcept = 0;

    virtual void      listen (int backlog) = 0;
    virtual CodeError accept (BackendStream* client) = 0;

    void handle_connection (const CodeError* err) {
        ltry([&]{ listener->handle_connection(err); });
    }

    IStreamListener* listener;
};

}}}
