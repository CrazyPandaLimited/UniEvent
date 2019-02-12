#include "TCP.h"
#include "Prepare.h"
#include "ssl/SSLFilter.h"

namespace panda { namespace unievent {

AddrInfoHintsSP TCP::default_hints = AddrInfoHintsSP(new AddrInfoHints);

TCP::~TCP() {
    _EDTOR();
}

TCP::TCP(Loop* loop, bool use_cached_resolver) : cached_resolver(use_cached_resolver) {
    _ECTOR();
    int err = uv_tcp_init(_pex_(loop), &uvh);
    if (err)
        throw CodeError(err);
    _init(&uvh);
}

TCP::TCP(Loop* loop, unsigned int flags, bool use_cached_resolver) : cached_resolver(use_cached_resolver) {
    _ECTOR();
    int err = uv_tcp_init_ex(_pex_(loop), &uvh, flags);
    if (err)
        throw CodeError(err);
    _init(&uvh);
}

StreamSP TCP::on_create_connection () {
    return new TCP(loop());
}

void TCP::open(sock_t socket) {
    int err = uv_tcp_open(&uvh, socket);
    if (err)
        throw CodeError(err);
}

void TCP::bind(const SockAddr& sa, unsigned int flags) {
    int err = uv_tcp_bind(&uvh, sa.get(), flags);
    if (err) throw CodeError(err);
}

void TCP::bind(std::string_view host, uint16_t port, const AddrInfoHintsSP& hints) {
    PEXS_NULL_TERMINATE(host, host_cstr);

    addrinfo h = hints->to<addrinfo>();
    addrinfo* res;
    int       syserr = getaddrinfo(host_cstr, string::from_number(port).c_str(), &h, &res);
    if (syserr)
        throw CodeError(_err_gai2uv(syserr));

    try {
        bind(res->ai_addr);
    } catch (...) {
        freeaddrinfo(res);
        throw;
    }
    freeaddrinfo(res);
}

void TCP::do_connect(TCPConnectRequest* req) {
    if (!req->sa) {
        _EDEBUGTHIS("do_connect, resolving %p", req);
        try {
            resolve_request = resolver()->resolve(req->host, string::from_number(req->port), req->hints,
                [=](SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
                    _EDEBUG("resolve callback, err: %d", err ? err->code() : 0);
                    resolve_request.reset();
                    if (err) {
                        int errcode = err->code();
                        Prepare::call_soon([=] { filters_.on_connect(CodeError(errcode), req); }, loop());
                        return;
                    }
                    req->sa = address->head->ai_addr;
                    do_connect(req);
                }, cached_resolver);
            return;
        } catch (...) {
            Prepare::call_soon([=] { filters_.on_connect(CodeError(ERRNO_RESOLVE), req); }, loop());
            return;
        }
    }

    int err = uv_tcp_connect(_pex_(req), &uvh, req->sa.get(), Stream::uvx_on_connect);
    if (err) {
        Prepare::call_soon([=] { filters_.on_connect(CodeError(err), req); }, loop());
        return;
    }
}

TCPConnectAutoBuilder TCP::connect() { return TCPConnectAutoBuilder(this); }

void TCP::connect(TCPConnectRequest* tcp_connect_request) {
    _EDEBUGTHIS("connect %p", tcp_connect_request);

    if (tcp_connect_request->is_reconnect) {
        disconnect();
    }

    _pex_(tcp_connect_request)->handle = _pex_(this);

    if (async_locked()) {
        asyncq_push(new CommandConnect(this, tcp_connect_request));
        return;
    }
    
    set_connecting();
    async_lock();
    retain();
    
    tcp_connect_request->retain();

    if (tcp_connect_request->timeout) {
        _EDEBUGTHIS("going to set timer to: %lu", tcp_connect_request->timeout);
        tcp_connect_request->set_timer(Timer::once(tcp_connect_request->timeout,
                                                    [=](Timer* t) {
                                                        _EDEBUG("connect timed out %p %p %u", t, this, tcp_connect_request->refcnt());
                                                        tcp_connect_request->release_timer();
                                                        cancel_connect();
                                                    },
                                                    loop()));
    }

    filters_.connect(tcp_connect_request);
}

void TCP::connect(const string& host, uint16_t port, uint64_t timeout, const AddrInfoHintsSP& hints) {
    _EDEBUGTHIS("connect to %.*s:%d", (int)host.length(), host.data(), port);
    connect().to(host, port, hints).timeout(timeout);
}

void TCP::connect (const SockAddr& sa, uint64_t timeout) {
    _EDEBUGTHIS("connect to sock:%s:%d", sa.ip().c_str(), sa.port());
    connect().to(sa).timeout(timeout);
}

void TCP::reconnect (TCPConnectRequest* tcp_connect_request) {
    tcp_connect_request->is_reconnect = true;
    connect(tcp_connect_request);
}

void TCP::reconnect (const SockAddr& sa, uint64_t timeout) {
    connect().to(sa).timeout(timeout).reconnect(true);
}

void TCP::reconnect (const string& host, uint16_t port, uint64_t timeout, const AddrInfoHintsSP& hints) {
    connect().to(host, port, hints).timeout(timeout).reconnect(true);
}

void TCP::on_handle_reinit () {
    int err = uv_tcp_init(uvh.loop, &uvh);
    if (err) throw CodeError(err);
    Stream::on_handle_reinit();
}

void TCP::_close() {
    _EDEBUGTHIS("close");
    if (resolve_request) {
        // ignore callback
        resolve_request->cancel();
        resolve_request.reset();
    }
    Stream::_close();
}

std::ostream& operator<< (std::ostream& os, const TCP& tcp) {
    return os << "local:" << tcp.get_sockaddr() << " peer:" << tcp.get_peer_sockaddr() << " connected:" << (tcp.connected() ? "yes" : "no");
}

std::ostream& operator<< (std::ostream& os, const TCPConnectRequest& r) {
    if (r.sa) return os << r.sa;
    else      return os << r.host << ':' << r.port;
}

}} // namespace panda::event
