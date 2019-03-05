//#pragma once
//#include "inc.h"
//#include "error.h"
//#include "XSCallback.h"
//#include <panda/unievent/Resolver.h>
//
//namespace xs { namespace unievent {
//
//using xs::my_perl;
//using namespace panda::unievent;
//
//struct XSResolver : Resolver {
//    XSCallback resolve_xscb;
//
//    using Resolver::Resolver;
//
//protected:
//    void on_resolve (const Resolver::RequestSP& req, const AddrInfo& addr, const CodeError* err) override;
//};
//
//}}
//
//namespace xs {
//    template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : TypemapObject<panda::unievent::Resolver*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
//        std::string package () { return "UniEvent::Resolver"; }
//    };
//}
