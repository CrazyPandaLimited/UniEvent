#include <xs/unievent/XSRequests.h>
#include <xs/unievent/XSHandles.h>
#include <panda/unievent/UDP.h>
#include <panda/unievent/Stream.h>

using xs::my_perl;
using namespace xs::unievent;

void XSConnectRequest::_cb (Stream* handle, const CodeError* err, ConnectRequest* req) {
    static_cast<XSConnectRequest*>(req)->xscb.call(xs::out(handle), nullptr, { xs::out(err) });
}

void XSTCPConnectRequest::_cb (Stream* handle, const CodeError* err, ConnectRequest* req) {
    static_cast<XSTCPConnectRequest*>(req)->xscb.call(xs::out(handle), nullptr, { xs::out(err) });
}

void XSShutdownRequest::_cb (Stream* handle, const CodeError* err, ShutdownRequest* req) {
    static_cast<XSShutdownRequest*>(req)->xscb.call(xs::out(handle), nullptr, { xs::out(err) });
}

void XSWriteRequest::_cb (Stream* handle, const CodeError* err, WriteRequest* req) {
    static_cast<XSWriteRequest*>(req)->xscb.call(xs::out(handle), nullptr, { xs::out(err) });
}

void XSSendRequest::_cb (UDP* handle, const CodeError* err, SendRequest* req) {
    static_cast<XSSendRequest*>(req)->xscb.call(xs::out(handle), nullptr, { xs::out(err) });
}
