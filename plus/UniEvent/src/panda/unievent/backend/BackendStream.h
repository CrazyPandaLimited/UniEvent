#pragma once
#include "BackendHandle.h"
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent { namespace backend {

struct IStreamListener {
    virtual string buf_alloc         (size_t cap) = 0;
    virtual void   handle_connection (const CodeError*) = 0;
    virtual void   handle_read       (string&, const CodeError*) = 0;
    virtual void   handle_eof        () = 0;
};

struct IConnectListener {
    virtual void handle_connect (const CodeError*) = 0;
};

struct BackendConnectRequest : BackendRequest {
    BackendConnectRequest (IConnectListener* l) : listener(l) {}

    void handle_connect (const CodeError* err) noexcept {
        ltry([&]{ listener->handle_connect(err); });
    }

    IConnectListener* listener;
};

struct IShutdownListener {
    virtual void handle_shutdown (const CodeError*) = 0;
};

struct BackendShutdownRequest : BackendRequest {
    BackendShutdownRequest (IShutdownListener* l) : listener(l) {}

    void handle_shutdown (const CodeError* err) noexcept {
        ltry([&]{ listener->handle_shutdown(err); });
    }

    IShutdownListener* listener;
};

struct BackendStream : BackendHandle {
    BackendStream (IStreamListener* l) : listener(l) {}

    string buf_alloc (size_t size) noexcept { return BackendHandle::buf_alloc(size, listener); }

    virtual bool readable () const noexcept = 0;
    virtual bool writable () const noexcept = 0;

    virtual void      listen     (int backlog) = 0;
    virtual CodeError accept     (BackendStream* client) = 0;
    virtual CodeError read_start () = 0;
    virtual void      read_stop  () = 0;
    virtual CodeError shutdown   (BackendShutdownRequest*) = 0;

    void handle_connection (const CodeError* err) noexcept {
        ltry([&]{ listener->handle_connection(err); });
    }

    void handle_read (string& buf, const CodeError* err) noexcept {
        ltry([&]{ listener->handle_read(buf, err); });
    }

    void handle_eof () noexcept {
        ltry([&]{ listener->handle_eof(); });
    }

    IStreamListener* listener;
};

}}}
