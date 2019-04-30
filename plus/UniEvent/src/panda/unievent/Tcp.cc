#include "Tcp.h"
#include "util.h"
//#include "ssl/SSLFilter.h"

namespace panda { namespace unievent {

const HandleType Tcp::TYPE("tcp");

AddrInfoHints Tcp::defhints = AddrInfoHints(AF_UNSPEC, SOCK_STREAM, 0, AddrInfoHints::PASSIVE);

Tcp::Tcp (const LoopSP& loop, int domain) : domain(domain) {
    _ECTOR();
    _init(loop, loop->impl()->new_tcp(this, domain));
}

const HandleType& Tcp::type () const {
    return TYPE;
}

backend::BackendHandle* Tcp::new_impl () {
    return loop()->impl()->new_tcp(this, domain);
}

void Tcp::open (sock_t sock, Ownership ownership) {
    if (ownership == Ownership::SHARE) sock = sock_dup(sock);
    impl()->open(sock);
    if (peeraddr()) {
        auto err = set_connect_result(true);
        if (err) throw err;
    }
}

void Tcp::bind (const net::SockAddr& addr, unsigned flags) {
    impl()->bind(addr, flags);
}

void Tcp::bind (std::string_view host, uint16_t port, const AddrInfoHints& hints, unsigned flags) {
    auto ai = sync_resolve(loop()->backend(), host, port, hints);
    bind(ai.addr(), flags);
}

StreamSP Tcp::create_connection () {
    return new Tcp(loop());
}

void Tcp::connect (const TcpConnectRequestSP& req) {
    req->set(this);
    queue.push(req);
}

void TcpConnectRequest::exec () {
    _EDEBUGTHIS();
    ConnectRequest::exec();
    if (handle->filters().size()) handle->filters().front()->tcp_connect(this);
    else                          finalize_connect();
}

void TcpConnectRequest::finalize_connect () {
    _EDEBUGTHIS();

    if (addr) {
        auto err = handle->impl()->connect(addr, impl());
        if (err) delay([=]{ cancel(err); });
        return;
    }

    resolve_request = handle->loop()->resolver()->resolve()
        ->node(host)
        ->port(port)
        ->hints(hints)
        ->use_cache(cached)
        ->on_resolve([this](const AddrInfo& res, const CodeError& res_err, const Resolver::RequestSP) {
            resolve_request = nullptr;
            if (res_err) return cancel(res_err);
            auto err = handle->impl()->connect(res.addr(), impl());
            if (err) cancel(err);
        });
    resolve_request->run();
}

void TcpConnectRequest::handle_event (const CodeError& err) {
    if (resolve_request) {
        resolve_request->event.remove_all();
        resolve_request->cancel();
        resolve_request = nullptr;
    }
    ConnectRequest::handle_event(err);
}


std::ostream& operator<< (std::ostream& os, const Tcp& tcp) {
    return os << "local:" << tcp.sockaddr() << " peer:" << tcp.peeraddr() << " connected:" << (tcp.connected() ? "yes" : "no");
}

std::ostream& operator<< (std::ostream& os, const TcpConnectRequest& r) {
    if (r.addr) return os << r.addr;
    else        return os << r.host << ':' << r.port;
}

}}
