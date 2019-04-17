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
    BackendConnectRequest (BackendHandle* h, IConnectListener* l) : BackendRequest(h), listener(l) {}

    void handle_connect (const CodeError* err) noexcept {
        handle->loop->ltry([&]{ listener->handle_connect(err); });
    }

    IConnectListener* listener;
};

struct IWriteListener {
    virtual void handle_write (const CodeError*) = 0;
};

struct BackendWriteRequest : BackendRequest {
    BackendWriteRequest (BackendHandle* h, IWriteListener* l) : BackendRequest(h), listener(l) {}

    void handle_write (const CodeError* err) noexcept {
        handle->loop->ltry([&]{ listener->handle_write(err); });
    }

    IWriteListener* listener;
};

struct IShutdownListener {
    virtual void handle_shutdown (const CodeError*) = 0;
};

struct BackendShutdownRequest : BackendRequest {
    BackendShutdownRequest (BackendHandle* h, IShutdownListener* l) : BackendRequest(h), listener(l) {}

    void handle_shutdown (const CodeError* err) noexcept {
        handle->loop->ltry([&]{ listener->handle_shutdown(err); });
    }

    IShutdownListener* listener;
};

struct BackendStream : BackendHandle {
    BackendStream (BackendLoop* loop, IStreamListener* lst) : BackendHandle(loop), listener(lst) {}

    virtual BackendConnectRequest*  new_connect_request  (IConnectListener*)  = 0;
    virtual BackendWriteRequest*    new_write_request    (IWriteListener*)    = 0;
    virtual BackendShutdownRequest* new_shutdown_request (IShutdownListener*) = 0;

    string buf_alloc (size_t size) noexcept { return BackendHandle::buf_alloc(size, listener); }

    virtual void      listen     (int backlog) = 0;
    virtual CodeError accept     (BackendStream* client) = 0;
    virtual CodeError read_start () = 0;
    virtual void      read_stop  () = 0;
    virtual CodeError write      (const std::vector<string>& bufs, BackendWriteRequest*) = 0;
    virtual CodeError shutdown   (BackendShutdownRequest*) = 0;

    virtual optional<fd_t> fileno () const = 0;

    virtual int  recv_buffer_size () const    = 0;
    virtual void recv_buffer_size (int value) = 0;
    virtual int  send_buffer_size () const    = 0;
    virtual void send_buffer_size (int value) = 0;

    virtual bool readable () const noexcept = 0;
    virtual bool writable () const noexcept = 0;

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
