#pragma once
#include <xs/unievent/error.h>
#include <xs/unievent/XSCallback.h>
#include <panda/unievent/Request.h>
#include <panda/unievent/TCP.h>

namespace xs { namespace unievent {

using xs::my_perl;
using namespace panda::unievent;

// The following classes are not visible from perl, XS wrappers are just needed for holding perl callback

class XSConnectRequest : public ConnectRequest {
public:
    XSCallback xscb;
    XSConnectRequest (pTHX_ SV* callback) : ConnectRequest(callback ? _cb : connect_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const CodeError* err, ConnectRequest* req);
};

class XSTCPConnectRequest : public TCPConnectRequest {
public:
    XSCallback xscb;

    XSTCPConnectRequest(bool            reconnect,
                        const sockaddr* sa,
                        const string&   host,
                        const string&   service,
                        const addrinfo* hints,
                        uint64_t        timeout,
                        SV*      xs_callback,
                        const SocksSP& socks)
            : TCPConnectRequest(reconnect, sa, host, service, hints, timeout, xs_callback ? _cb : connect_fn(nullptr), socks) {
        xscb.set(xs_callback);
    }

    class Builder : public BasicBuilder<Builder> {
    public:
        Builder& callback(SV* xs_callback) { xs_callback_ = xs_callback; return *this; }

        XSTCPConnectRequest* build() {
            return new XSTCPConnectRequest(reconnect_, sa_, host_, service_, hints_, timeout_, xs_callback_, socks_);
        }

    private:
        SV* xs_callback_ = nullptr;
    };

private:
    static void _cb (Stream* handle, const CodeError* err, ConnectRequest* req);
};


class XSShutdownRequest : public ShutdownRequest {
public:
    XSCallback xscb;
    XSShutdownRequest (pTHX_ SV* callback) : ShutdownRequest(callback ? _cb : shutdown_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const CodeError* err, ShutdownRequest* req);
};


class XSWriteRequest : public WriteRequest {
public:
    XSCallback xscb;
    XSWriteRequest (pTHX_ SV* callback) : WriteRequest(callback ? _cb : write_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const CodeError* err, WriteRequest* req);
};


class XSSendRequest : public SendRequest {
public:
    XSCallback xscb;
    XSSendRequest (pTHX_ SV* callback) : SendRequest(callback ? _cb : send_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (UDP* handle, const CodeError* err, SendRequest* req);
};


}}
