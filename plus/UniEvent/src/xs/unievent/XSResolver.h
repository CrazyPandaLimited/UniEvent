#pragma once
#include "inc.h"
#include "error.h"
#include "XSCallback.h"
#include <panda/unievent/Resolver.h>

namespace xs { namespace unievent {

using xs::my_perl;
using namespace panda::unievent;

struct XSResolver : Resolver {
    XSCallback resolve_xscb;

    using Resolver::Resolver;

protected:
    void on_resolve (const ResolveRequestSP& req, const AddrInfo& addr, const CodeError* err) override {
        _EDEBUGTHIS();
        auto salistref = Scalar::undef;
        if (!err) {
            auto salist = Array::create();
            for (auto ai = addr; ai; ai = ai.next()) {
                salist.push(xs::out(ai.addr()));
            }
            salistref = Ref::create(salist);
        }
        resolve_xscb.call(xs::out<Resolver*>(this), evname_on_resolve, {salistref, xs::out(err)});
        Resolver::on_resolve(req, addr, err);
    }

    static const Simple evname_on_resolve;
};

}}

namespace xs {
    template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
        std::string package () { return "UniEvent::Resolver"; }
    };
}
