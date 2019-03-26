//#pragma once
//#include "error.h"
//#include "XSCallback.h"
//#include <panda/unievent/TCP.h>
//#include <panda/unievent/Request.h>
//
//namespace xs { namespace unievent {
//
//using xs::my_perl;
//using namespace panda::unievent;
//
//// The following classes are not visible from perl, XS wrappers are just needed for holding perl callback
//
//struct XSConnectRequest : ConnectRequest {
//    XSCallback xscb;
//    XSConnectRequest (pTHX_ SV* callback) : ConnectRequest(callback ? _cb : connect_fn(nullptr)) {
//        xscb.set(callback);
//    }
//private:
//    static void _cb (Stream* handle, const CodeError* err, ConnectRequest* req);
//};
//
//struct XSTCPConnectRequest : TCPConnectRequest {
//    XSCallback xscb;
//
//    XSTCPConnectRequest(
//        bool reconnect, const SockAddr& sa, const string& host, uint16_t port, const AddrInfoHintsSP& hints, uint64_t timeout, SV* xs_callback)
//            : TCPConnectRequest(reconnect, sa, host, port, hints, timeout, xs_callback ? _cb : connect_fn(nullptr)) {
//        xscb.set(xs_callback);
//    }
//
//    struct Builder : BasicBuilder<Builder> {
//        Builder& callback (SV* xs_callback) { _xs_callback = xs_callback; return *this; }
//
//        XSTCPConnectRequest* build () {
//            return new XSTCPConnectRequest(_reconnect, _sa, _host, _port, _hints, _timeout, _xs_callback);
//        }
//
//    private:
//        SV* _xs_callback = nullptr;
//    };
//
//private:
//    static void _cb (Stream* handle, const CodeError* err, ConnectRequest* req);
//};
//
//
//struct XSShutdownRequest : ShutdownRequest {
//    XSCallback xscb;
//    XSShutdownRequest (pTHX_ SV* callback) : ShutdownRequest(callback ? _cb : shutdown_fn(nullptr)) {
//        xscb.set(callback);
//    }
//private:
//    static void _cb (Stream* handle, const CodeError* err, ShutdownRequest* req);
//};
//
//
//struct XSWriteRequest : WriteRequest {
//    XSCallback xscb;
//    XSWriteRequest (pTHX_ SV* callback) : WriteRequest(callback ? _cb : write_fn(nullptr)) {
//        xscb.set(callback);
//    }
//private:
//    static void _cb (Stream* handle, const CodeError* err, WriteRequest* req);
//};
//
//}}
