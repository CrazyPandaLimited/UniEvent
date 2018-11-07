#pragma once
#include "error.h"
#include "XSCallback.h"
#include <panda/unievent/TCP.h>
#include <panda/unievent/Request.h>
#include <panda/unievent/Resolver.h>

namespace xs { namespace unievent {

using xs::my_perl;
using namespace panda::unievent;

struct XSResolver : Resolver {
    XSCallback resolve_xscb;
    using Resolver::Resolver;

protected:
    void on_resolve (SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err) override {
        _EDEBUGTHIS();
        auto salistref = Scalar::undef;
        if (!err) {
            auto salist = Array::create();
            for (auto ai = address->head; ai; ai = ai->ai_next) {
                salist.push(xs::out<SockAddr>(ai->ai_addr));
            }
            salistref = Ref::create(salist);
        }
        resolve_xscb.call(xs::out<Resolver*>(this), evname_on_resolve, {salistref, xs::out(err)});
        Resolver::on_resolve(resolver, resolve_request, address, err);
    }

    static const Simple evname_on_resolve;
};

const Simple XSResolver::evname_on_resolve = Simple::shared("on_resolve");

}} // namespace xs::unievent

namespace xs {
    template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
        std::string package () { return "UniEvent::Resolver"; }
    };
}
