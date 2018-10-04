#pragma once
#include <panda/unievent/Request.h>
#include <panda/unievent/Resolver.h>
#include <panda/unievent/TCP.h>
#include <xs/unievent/XSCallback.h>
#include <xs/unievent/error.h>

namespace xs { namespace unievent {

using xs::my_perl;
using namespace panda::unievent;

template <class T> struct BasicXSResolver : T {
    XSCallback resolve_xscb;
    using T::T;

protected:
    void on_resolve (AbstractResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) {
        auto salistref = Scalar::undef;
        if (!err) {
            auto salist = Array::create();
            for (addrinfo* ai = address->head; ai; ai = ai->ai_next) {
                sockaddr* sa = ai->ai_addr;
                auto sastr = std::string_view((char*)sa, sa->sa_family == PF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in));
                salist.push(Simple(sastr));
            }
            salistref = Ref::create(salist);
        }
        if (!resolve_xscb.call(xs::out<T*>(this), evname_on_resolve, {salistref, xs::out(err)})) {
            T::on_resolve(resolver, resolve_request, address, err);
        }
    }

    static const Simple evname_on_resolve;
};

template <class T>
const Simple BasicXSResolver<T>::evname_on_resolve = Simple::shared("on_resolve");

using XSResolver       = BasicXSResolver<Resolver>;
using XSCachedResolver = BasicXSResolver<CachedResolver>;

}} // namespace xs::event

namespace xs {

    template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
        std::string package () { return "UniEvent::Resolver"; }
    };

    template <class TYPE> struct Typemap<panda::unievent::CachedResolver*, TYPE> : TypemapObject<panda::unievent::CachedResolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
        std::string package () { return "UniEvent::CachedResolver"; }
    };

}