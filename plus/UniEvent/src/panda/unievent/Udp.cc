#include "Udp.h"
#include "SyncLoop.h"
#include "Resolver.h"
using namespace panda::unievent;

const HandleType Udp::TYPE("udp");

AddrInfoHints Udp::defhints = AddrInfoHints(AF_UNSPEC, SOCK_DGRAM, 0, AddrInfoHints::PASSIVE);

static SyncLoop sloop;

const HandleType& Udp::type () const {
    return TYPE;
}

string Udp::buf_alloc (size_t cap) {
    return buf_alloc_callback ? buf_alloc_callback(cap) : string(cap);
}

void Udp::open (sock_t socket) {
    impl()->open(socket);
}

void Udp::bind (const net::SockAddr& sa, unsigned flags) {
    impl()->bind(sa, flags);
}

void Udp::bind (string_view host, uint16_t port, const AddrInfoHints& hints, unsigned flags) {
    auto l = sloop.get(loop()->backend());
    AddrInfo ai;
    l->resolver()->resolve()->node(string(host))->port(port)->hints(hints)->on_resolve([&ai](const AddrInfo& res, const CodeError* err, const Resolver::RequestSP) {
        if (err) throw *err;
        ai = res;
    })->run();
    l->run();

    bind(ai.addr(), flags);
}

void Udp::set_membership (std::string_view multicast_addr, std::string_view interface_addr, Membership membership) {
    impl()->set_membership(multicast_addr, interface_addr, membership);
}

void Udp::set_multicast_loop (bool on) {
    impl()->set_multicast_loop(on);
}

void Udp::set_multicast_ttl (int ttl) {
    impl()->set_multicast_ttl(ttl);
}

void Udp::set_multicast_interface (std::string_view interface_addr) {
    impl()->set_multicast_interface(interface_addr);
}

void Udp::set_broadcast (bool on) {
    impl()->set_broadcast(on);
}

void Udp::set_ttl (int ttl) {
    impl()->set_ttl(ttl);
}

void Udp::recv_start (receive_fn callback) {
    if (callback) receive_event.add(callback);
    impl()->recv_start();
}

void Udp::recv_stop () {
    impl()->recv_stop();
}

void Udp::send (const SendRequestSP& req) {
    req->set(this);
    queue.push(req);
}

void Udp::SendRequest::exec () {
    auto err = handle->impl()->send(bufs, addr, impl());
    if (err) delay_id = handle->loop()->delay([=]{ handle_send(err); });
}

void Udp::SendRequest::on_cancel () {
    handle->on_send(CodeError(std::errc::operation_canceled), this);
}

void Udp::SendRequest::handle_send (const CodeError* err) {
    SendRequestSP self = this;
    handle->queue.next();
    handle->on_send(err, self);
}

void Udp::on_send (const CodeError* err, const SendRequestSP& r) {
    r->event(this, err, r);
}

void Udp::reset () {
    scope_guard([&] {
        queue.abort();
        impl()->destroy();
        _impl = loop()->impl()->new_udp(this, domain);
    }, [&] {
        queue.resume();
    });
}

void Udp::handle_receive (string& buf, const net::SockAddr& sa, unsigned flags, const CodeError* err) {
    on_receive(buf, sa, flags, err);
}

void Udp::on_receive (string& buf, const net::SockAddr& sa, unsigned flags, const CodeError* err) {
    receive_event(this, buf, sa, flags, err);
}
