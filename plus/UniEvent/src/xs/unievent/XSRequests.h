#pragma once
#include "error.h"
#include "XSCallback.h"
#include <panda/unievent/TCP.h>
#include <panda/unievent/Request.h>

namespace xs { namespace unievent {

using xs::my_perl;
using namespace panda::unievent;

// The following classes are not visible from perl, XS wrappers are just needed for holding perl callback

struct XSConnectRequest : ConnectRequest {
    XSCallback xscb;
    XSConnectRequest (pTHX_ SV* callback) : ConnectRequest(callback ? _cb : connect_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const CodeError* err, ConnectRequest* req);
};

struct XSTCPConnectRequest : TCPConnectRequest {
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


struct XSShutdownRequest : ShutdownRequest {
    XSCallback xscb;
    XSShutdownRequest (pTHX_ SV* callback) : ShutdownRequest(callback ? _cb : shutdown_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const CodeError* err, ShutdownRequest* req);
};


struct XSWriteRequest : WriteRequest {
    XSCallback xscb;
    XSWriteRequest (pTHX_ SV* callback) : WriteRequest(callback ? _cb : write_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const CodeError* err, WriteRequest* req);
};


struct XSSendRequest : SendRequest {
    XSCallback xscb;
    XSSendRequest (pTHX_ SV* callback) : SendRequest(callback ? _cb : send_fn(nullptr)) {
        xscb.set(callback);
    }
private:
    static void _cb (UDP* handle, const CodeError* err, SendRequest* req);
};


}}
