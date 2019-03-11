#include "XSResolver.h"

//namespace xs { namespace unievent {
//
//static const Simple evname_on_resolve = Simple::shared("on_resolve");
//
//void XSResolver::on_resolve (const Resolver::RequestSP& req, const AddrInfo& addr, const CodeError* err) {
//    _EDEBUGTHIS();
//    auto salistref = Scalar::undef;
//    if (!err) {
//        auto salist = Array::create();
//        for (auto ai = addr; ai; ai = ai.next()) {
//            salist.push(xs::out(ai.addr()));
//        }
//        salistref = Ref::create(salist);
//    }
//    resolve_xscb.call(xs::out<Resolver*>(this), evname_on_resolve, {salistref, xs::out(err)});
//    Resolver::on_resolve(req, addr, err);
//}
//
//}}
