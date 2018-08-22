#pragma once
#include <xs/unievent/inc.h>
#include <xs/unievent/error.h>
#include <xs/unievent/XSCallback.h>

namespace xs { namespace unievent {

using xs::my_perl;
using namespace panda::unievent;

struct XSResolver : Resolver {
public:
    XSCallback resolve_xscb;
    XSResolver (Loop* loop = Loop::default_loop()) : Resolver(loop), resolve_xscb(aTHX) {}
protected:
    virtual void on_resolve (struct addrinfo* res, const ResolveError& err);
};

// The following classes are not visible from perl, XS wrappers are just needed for holding perl callback

struct XSConnectRequest : ConnectRequest {
    XSCallback xscb;
    XSConnectRequest (pTHX_ SV* callback) : ConnectRequest(callback ? _cb : nullptr), xscb(aTHX) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const StreamError& err, ConnectRequest* req);
};


struct XSShutdownRequest : ShutdownRequest {
    XSCallback xscb;
    XSShutdownRequest (pTHX_ SV* callback) : ShutdownRequest(callback ? _cb : nullptr), xscb(aTHX) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const StreamError& err, ShutdownRequest* req);
};


struct XSWriteRequest : WriteRequest {
    XSCallback xscb;
    XSWriteRequest (pTHX_ SV* callback) : WriteRequest(callback ? _cb : nullptr), xscb(aTHX) {
        xscb.set(callback);
    }
private:
    static void _cb (Stream* handle, const StreamError& err, WriteRequest* req);
};


struct XSSendRequest : SendRequest {
    XSCallback xscb;
    XSSendRequest (pTHX_ SV* callback) : SendRequest(callback ? _cb : nullptr), xscb(aTHX) {
        xscb.set(callback);
    }
private:
    static void _cb (UDP* handle, const UDPError& err, SendRequest* req);
};


}}

namespace xs {

template <class TYPE>
struct Typemap<panda::unievent::Request*, TYPE> : TypemapObject<panda::unievent::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {
    std::string package () const { return "UniEvent::Request"; }
};

template <class TYPE> struct Typemap<panda::unievent::Resolver*, TYPE> : Typemap<panda::unievent::Request*, TYPE> {
    std::string package () const { return "UniEvent::Resolver"; }
};

}
