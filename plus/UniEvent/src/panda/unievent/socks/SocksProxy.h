#pragma once
/*
#include <cstdint>

#include <panda/unievent/Request.h>
#include <panda/unievent/Socks.h>
#include <panda/lib/memory.h>
#include <panda/refcnt.h>

namespace panda { namespace unievent {
class ResolveRequest;
class ConnectRequest;
class TCP;
}} // namespace panda::event

namespace panda { namespace unievent { namespace socks {

class SocksRequest;
class SocksConnectRequest;
class SocksWriteRequest;
class SocksProxy : public virtual Refcnt, public lib::AllocatedObject<SocksProxy, true> {
public:
    enum class State {
        resolve_proxy    = 0, // going to resolve proxy
        resolving_proxy  = 1, // cancelable async resolve
        resolve_host     = 2, // going to resolve host
        resolving_host   = 3, // cancelable async resolve
        connect_proxy    = 4, // going to connect proxy
        connecting_proxy = 5, // connecting, not cancelable
        handshake        = 6, // going to handshake
        handshake_write  = 7, // handshaking, not cancelable
        handshake_reply  = 8, // waiting for reply, not cancelable
        auth             = 9,
        auth_write       = 10,
        auth_reply       = 11,
        connect          = 12,
        connect_write    = 13,
        connect_reply    = 14,
        connected        = 15, // success
        parsing          = 16, // something not parsed yet
        eof              = 17,
        terminal         = 18,
        canceled         = 19, // timeout happened
        error            = 20
    };

    static const char* TYPE;

    virtual ~SocksProxy();
    SocksProxy(TCP* tcp, SocksSP socks);

    bool start(const string& host, const string& service, const addrinfo* hints, ConnectRequestSP req);

    bool start(const sockaddr* sa, ConnectRequestSP req);

    void cancel();

    void do_transition(State s);

private:
    void do_write(SocksWriteRequest* req);

    void do_resolve_proxy();
    void do_connect_proxy();
    void do_handshake();
    void do_auth();
    void do_resolve_host();
    void do_connect();
    void do_connected();
    void do_eof();
    void do_error();
    void do_close();

    void  state(State s) { state_ = s; }
    State state() const { return state_; }

    void init_parser();

    void print_pointers() const;

    static void uvx_on_connect(uv_connect_t* uvreq, int status);
    static void uvx_on_close(uv_handle_t* uvh);
    static void uvx_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void uvx_on_write(uv_write_t* uvreq, int status);

private:
    TCP*             tcp_;
    SocksSP          socks_;
    State            state_;
    string           host_;
    uint16_t         port_;
    bool             host_resolved_;
    addrinfo         hints_;
    sockaddr_storage addr_;
    sockaddr_storage socks_addr_;

    iptr<ConnectRequest>      connect_request_;
    iptr<SocksConnectRequest> socks_connect_request_;
    iptr<ResolveRequest>      resolve_request_;
    iptr<Request>             current_request_;

    int     cs;
    bool    noauth;
    uint8_t auth_status;
    uint8_t atyp;
    uint8_t rep;
};

using SocksProxySP = iptr<SocksProxy>;

class SocksConnectRequest : public Request, public AllocatedObject<ConnectRequest, true> {
    friend uv_connect_t* _pex_(SocksConnectRequest*);

public:
    using connect_fptr = void(TCP* handle, const CodeError* err, SocksConnectRequest* req);
    using connect_fn   = function<connect_fptr>;

    ~SocksConnectRequest();
    SocksConnectRequest(connect_fn callback);

    Handle* handle() { return static_cast<Handle*>(uvr_.handle->data); }

public:
    CallbackDispatcher<connect_fptr> event;

private:
    uv_connect_t uvr_;
};

class SocksWriteRequest : public Request {
    friend SocksProxy;
    friend uv_write_t* _pex_(SocksWriteRequest*);

public:
    using write_fptr = void(TCP* handle, const CodeError* err, SocksWriteRequest* req);
    using write_fn   = function<write_fptr>;

    ~SocksWriteRequest() { _EDEBUGTHIS("dtor"); }

    SocksWriteRequest(write_fn callback) {
        _EDEBUGTHIS("ctor");
        _init(&uvr_);
        event.add(callback);
    }

    Handle* handle() const { return static_cast<Handle*>(uvr_.handle->data); }

public:
    CallbackDispatcher<write_fptr> event;

protected:
    uv_write_t uvr_;
    string     buffer_;
};

class SocksHandshakeRequest : public SocksWriteRequest, public AllocatedObject<SocksHandshakeRequest, true> {
    friend uv_write_t* _pex_(SocksHandshakeRequest*);

public:
    ~SocksHandshakeRequest() { _EDEBUGTHIS("dtor"); }

    SocksHandshakeRequest(write_fn callback, const SocksSP socks)
            : SocksWriteRequest(callback) {
        _EDEBUGTHIS("ctor");

        if (socks->loginpassw()) {
            buffer_ = "\x05\x02\x00\x02";
        } else {
            buffer_ = "\x05\x01\x00";
        }
    }
};

class SocksAuthRequest : public SocksWriteRequest, public AllocatedObject<SocksAuthRequest, true> {
    friend uv_write_t* _pex_(SocksAuthRequest*);

public:
    ~SocksAuthRequest() { _EDEBUGTHIS("dtor"); }

    SocksAuthRequest(write_fn callback, const SocksSP socks)
            : SocksWriteRequest(callback) {
        _EDEBUGTHIS("ctor");
        buffer_ = string("\x05") + (char)socks->login.length() + socks->login + (char)socks->passw.length() + socks->passw;
    }
};

class SocksCommandConnectRequest : public SocksWriteRequest, public AllocatedObject<SocksCommandConnectRequest, true> {
    friend uv_write_t* _pex_(SocksCommandConnectRequest*);

public:
    ~SocksCommandConnectRequest() { _EDEBUGTHIS("dtor"); }

    SocksCommandConnectRequest(write_fn callback, const string& host, uint16_t port)
            : SocksWriteRequest(callback) {
        _EDEBUGTHIS("ctor");
        _EDEBUGTHIS("connecting to: %.*s:%d", (int)host.length(), host.data(), port);
        uint16_t nport = htons(port);
        buffer_        = string("\x05\x01\x00\x03") + (char)host.length() + host + string((char*)&nport, 2);
    }

    SocksCommandConnectRequest(write_fn callback, const sockaddr* sa)
            : SocksWriteRequest(callback) {
        _EDEBUGTHIS("ctor");
        if (sa->sa_family == AF_INET) {
            sockaddr_in* src   = (sockaddr_in*)sa;
            uint16_t     nport = src->sin_port;
            buffer_            = string("\x05\x01\x00\x01") + string((char*)&src->sin_addr, 4) + string((char*)&nport, 2);
        } else if (sa->sa_family == AF_INET6) {
            sockaddr_in6* src   = (sockaddr_in6*)sa;
            uint16_t      nport = src->sin6_port;
            buffer_             = string("\x05\x01\x00\x04") + string((char*)&src->sin6_addr, 16) + string((char*)&nport, 2);
        } else {
            throw Error("Unknown address family");
        }
    }
};

inline uv_connect_t* _pex_(SocksConnectRequest* req) { return &req->uvr_; }
inline uv_write_t*   _pex_(SocksWriteRequest* req) { return &req->uvr_; }
inline uv_write_t*   _pex_(SocksHandshakeRequest* req) { return &req->uvr_; }

}}} // namespace panda::event::socks
*/
