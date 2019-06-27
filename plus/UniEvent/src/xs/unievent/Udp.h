#pragma once
#include "util.h"
#include "Error.h"
#include "Handle.h"
#include "Request.h"
#include "AddrInfo.h"
#include <xs/net/sockaddr.h>
#include <panda/unievent/Udp.h>

namespace xs { namespace unievent {

struct XSUdp : panda::unievent::Udp {
    using Udp::Udp;
protected:
    void on_receive (panda::string& buf, const panda::net::SockAddr& sa, unsigned flags, const panda::unievent::CodeError& err) override;
    void on_send    (const panda::unievent::CodeError& err, const panda::unievent::SendRequestSP& req) override;
};

}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::Udp*, TYPE> : Typemap<panda::unievent::BackendHandle*, TYPE> {
    static panda::string package () { return "UniEvent::Udp"; }
};

template <class TYPE> struct Typemap<panda::unievent::SendRequest*, TYPE> : Typemap<panda::unievent::Request*, TYPE> {
    static panda::string package () { return "UniEvent::Request::Send"; }
};

}
