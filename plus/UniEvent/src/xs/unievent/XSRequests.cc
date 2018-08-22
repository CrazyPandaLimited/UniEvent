#include <xs/unievent/XSRequests.h>
#include <xs/unievent/XSHandles.h>
#include <panda/unievent/UDP.h>
#include <panda/unievent/Stream.h>

namespace xs { namespace unievent {

static const auto evname_on_resolve = Simple::shared("on_resolve");

void XSResolver::on_resolve (struct addrinfo* res, const ResolveError& err) {
    auto obj = xs::out<Resolver*>(aTHX_ this);
    Scalar salistref;
    if (err) salistref = Scalar::undef;
    else {
        auto salist = Array::create();
        for (addrinfo* ai = res; ai; ai = ai->ai_next) salist.push(xs::out(aTHX_ ai->ai_addr));
        salistref = Ref::create(salist);
    }
    if (resolve_xscb.call(obj, evname_on_resolve, { salistref, error_sv(err) })) Resolver::free(res);
    else Resolver::on_resolve(res, err);
}

void XSConnectRequest::_cb (Stream* handle, const StreamError& err, ConnectRequest* req) {
    auto obj = xs::out(aTHX_ handle);
    static_cast<XSConnectRequest*>(req)->xscb.call(obj, nullptr, { error_sv(err) });
}

void XSShutdownRequest::_cb (Stream* handle, const StreamError& err, ShutdownRequest* req) {
    auto obj = xs::out(aTHX_ handle);
    static_cast<XSShutdownRequest*>(req)->xscb.call(obj, nullptr, { error_sv(err) });
}

void XSWriteRequest::_cb (Stream* handle, const StreamError& err, WriteRequest* req) {
    auto obj = xs::out(aTHX_ handle);
    static_cast<XSWriteRequest*>(req)->xscb.call(obj, nullptr, { error_sv(err) });
}

void XSSendRequest::_cb (UDP* handle, const UDPError& err, SendRequest* req) {
    auto obj = xs::out(aTHX_ handle);
    static_cast<XSSendRequest*>(req)->xscb.call(obj, nullptr, { error_sv(err) });
}

}}
