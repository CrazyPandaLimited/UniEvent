#include "Tcp.h"
#include "util.h"
//#include "ssl/SSLFilter.h"

using namespace panda::unievent;

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

void Tcp::open (sock_t sock) {
    impl()->open(sock);
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

//void TCP::do_connect(TCPConnectRequest* req) {
//    if (!req->sa) {
//        _EDEBUGTHIS("do_connect, resolving %p", req);
//        try {
//            resolve_request = resolver()->resolve(req->host, string::from_number(req->port), req->hints,
//                [=](SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
//                    _EDEBUG("resolve callback, err: %d", err ? err->code() : 0);
//                    resolve_request.reset();
//                    if (err) {
//                        int errcode = err->code();
//                        call_soon_or_on_reset([=] { filters_.on_connect(CodeError(errcode), req); });
//                        return;
//                    }
//                    req->sa = address->head->ai_addr;
//                    do_connect(req);
//                }, cached_resolver);
//            return;
//        } catch (...) {
//            call_soon_or_on_reset([=] { filters_.on_connect(CodeError(ERRNO_RESOLVE), req); });
//            return;
//        }
//    }
//
//    int err = uv_tcp_connect(_pex_(req), &uvh, req->sa.get(), Stream::uvx_on_connect);
//    if (err) {
//        call_soon_or_on_reset([=] {
//            filters_.on_connect(CodeError(err), req);
//        });
//        return;
//    }
//}
//
//TCPConnectAutoBuilder TCP::connect() { return TCPConnectAutoBuilder(this); }
//
//void TCP::connect(TCPConnectRequest* tcp_connect_request) {
//    _EDEBUGTHIS("connect %p", tcp_connect_request);
//
//    if (tcp_connect_request->is_reconnect) {
//        disconnect();
//    }
//
//    _pex_(tcp_connect_request)->handle = _pex_(this);
//
//    if (async_locked()) {
//        asyncq_push(new CommandConnect(this, tcp_connect_request));
//        return;
//    }
//
//    set_connecting();
//    async_lock();
//    retain();
//
//    tcp_connect_request->retain();
//
//    if (tcp_connect_request->timeout) {
//        _EDEBUGTHIS("going to set timer to: %lu", tcp_connect_request->timeout);
//        tcp_connect_request->set_timer(Timer::once(tcp_connect_request->timeout,
//                                                    [=](Timer* t) {
//                                                        _EDEBUG("connect timed out %p %p %u", t, this, tcp_connect_request->refcnt());
//                                                        tcp_connect_request->release_timer();
//                                                        cancel_connect();
//                                                    },
//                                                    loop()));
//    }
//
//    filters_.connect(tcp_connect_request);
//}
//
//void TCP::connect(const string& host, uint16_t port, uint64_t timeout, const AddrInfoHintsSP& hints) {
//    _EDEBUGTHIS("connect to %.*s:%d", (int)host.length(), host.data(), port);
//    connect().to(host, port, hints).timeout(timeout);
//}
//
//void TCP::connect (const SockAddr& sa, uint64_t timeout) {
//    _EDEBUGTHIS("connect to sock:%s:%d", sa.ip().c_str(), sa.port());
//    connect().to(sa).timeout(timeout);
//}
//
//void TCP::reconnect (TCPConnectRequest* tcp_connect_request) {
//    tcp_connect_request->is_reconnect = true;
//    connect(tcp_connect_request);
//}
//
//void TCP::reconnect (const SockAddr& sa, uint64_t timeout) {
//    connect().to(sa).timeout(timeout).reconnect(true);
//}
//
//void TCP::reconnect (const string& host, uint16_t port, uint64_t timeout, const AddrInfoHintsSP& hints) {
//    connect().to(host, port, hints).timeout(timeout).reconnect(true);
//}
//
//void TCP::on_handle_reinit () {
//    int err = uv_tcp_init(uvh.loop, &uvh);
//    if (err) throw CodeError(err);
//    Stream::on_handle_reinit();
//}
//
//void TCP::_close() {
//    _EDEBUGTHIS("close");
//    if (resolve_request) {
//        // ignore callback
//        resolve_request->cancel();
//        resolve_request.reset();
//    }
//    Stream::_close();
//}
//
//std::ostream& operator<< (std::ostream& os, const TCP& tcp) {
//    return os << "local:" << tcp.get_sockaddr() << " peer:" << tcp.get_peer_sockaddr() << " connected:" << (tcp.connected() ? "yes" : "no");
//}
//
//std::ostream& operator<< (std::ostream& os, const TCPConnectRequest& r) {
//    if (r.sa) return os << r.sa;
//    else      return os << r.host << ':' << r.port;
//}
