#include "TCP.h"
#include "Prepare.h"
#include "ssl/SSLFilter.h"

namespace panda { namespace unievent {

static addrinfo init_default_hints() {
    addrinfo ret;
    memset(&ret, 0, sizeof(ret));
    ret.ai_family   = PF_UNSPEC;
    ret.ai_socktype = SOCK_STREAM;
    ret.ai_flags    = AI_PASSIVE;
    return ret;
}

addrinfo TCP::defhints = init_default_hints();

TCP::~TCP() { _EDTOR(); _EDEBUG("~TCP: %p", static_cast<Handle*>(this)); }

TCP::TCP(Loop* loop, bool cached_resolver) {
    _ECTOR();
    _EDEBUG("TCP: %p", static_cast<Handle*>(this));
    int err = uv_tcp_init(_pex_(loop), &uvh);
    if (err)
        throw CodeError(err);
    _init(&uvh);

    init(cached_resolver);
}

TCP::TCP(Loop* loop, unsigned int flags, bool cached_resolver) {
    _ECTOR();
    int err = uv_tcp_init_ex(_pex_(loop), &uvh, flags);
    if (err)
        throw CodeError(err);
    _init(&uvh);

    init(cached_resolver);
}

void TCP::init(bool cached_resolver) {
    connection_factory = [=](){return new TCP(loop(), cached_resolver);};

    if (cached_resolver) {
        resolver = get_thread_local_cached_resolver();
    } else {
        resolver = get_global_basic_resolver();
    }
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

void TCP::bind(std::string_view host, std::string_view service, const addrinfo* hints) {
    if (!hints)
        hints = &defhints;

    PEXS_NULL_TERMINATE(host, host_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    addrinfo* res;
    int       syserr = getaddrinfo(host_cstr, service_cstr, hints, &res);
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
    if (!req->sa_) {
        _EDEBUGTHIS("do_connect, resolving %p", req);
        try {
            resolve_request = resolver->resolve(loop(), req->host_, req->service_, &req->hints_, [=](AbstractResolverSP, ResolveRequestSP, BasicAddressSP address, const CodeError* err) {
                _EDEBUG("resolve callback, err: %d", err ? err->code() : 0);
                if (err) {
                    int errcode = err->code();
                    Prepare::call_soon([=] { filters_.on_connect(CodeError(errcode), req); }, loop());
                    return;
                }
                req->sa_ = address->head->ai_addr;
                do_connect(req);
            });
            return;
        } catch (...) {
            Prepare::call_soon([=] { filters_.on_connect(CodeError(ERRNO_RESOLVE), req); }, loop());
            return;
        }
    }
    
    int err = uv_tcp_connect(_pex_(req), &uvh, req->sa_.get(), Stream::uvx_on_connect);
    if (err) {
        Prepare::call_soon([=] { filters_.on_connect(CodeError(err), req); }, loop());
        return;
    }
}

TCPConnectAutoBuilder TCP::connect() { return TCPConnectAutoBuilder(this); }

void TCP::connect(TCPConnectRequest* tcp_connect_request) {
    _EDEBUGTHIS("connect %p", tcp_connect_request);

    if(tcp_connect_request->is_reconnect) {
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

    if (tcp_connect_request->timeout_) {
        _EDEBUGTHIS("going to set timer to: %lu", tcp_connect_request->timeout_);
        tcp_connect_request->set_timer(Timer::once(tcp_connect_request->timeout_,
                                                    [=](Timer* t) {
                                                        _EDEBUG("connect timed out %p %p %u", t, this, tcp_connect_request->refcnt());
                                                        tcp_connect_request->release_timer();
                                                        cancel_connect();
                                                    },
                                                    loop()));
    }

    filters_.connect(tcp_connect_request);
}

void TCP::connect(const string& host, const string& service, uint64_t timeout, const addrinfo* hints) {
    _EDEBUGTHIS("connect to %.*s:%.*s", (int)host.length(), host.data(), (int)service.length(), service.data());
    connect().to(host, service, hints).timeout(timeout);
}

void TCP::connect(const SockAddr& sa, uint64_t timeout) {
    _EDEBUGTHIS("connect to sock:%p", sa);
    connect().to(sa).timeout(timeout);
}

void TCP::reconnect(TCPConnectRequest* tcp_connect_request) {
    tcp_connect_request->is_reconnect = true;
    connect(tcp_connect_request);
}

void TCP::reconnect(const SockAddr& sa, uint64_t timeout) {
    connect().to(sa).timeout(timeout).reconnect(true);
}

void TCP::reconnect(const string& host, const string& service, uint64_t timeout, const addrinfo* hints) {
    connect().to(host, service, hints).timeout(timeout).reconnect(true);
}

void TCP::on_handle_reinit() {
    int err = uv_tcp_init(uvh.loop, &uvh);
    if (err)
        throw CodeError(err);
    Stream::on_handle_reinit();
}

void TCP::_close() {
    _EDEBUGTHIS("close resolve_request:%p", resolve_request.get());

    if (resolve_request) {
        resolve_request->cancel();
    }

    Stream::_close();
}

std::ostream& operator<< (std::ostream& os, const TCP& tcp) {
    return os << "local:" << tcp.get_sockaddr() << " peer:" << tcp.get_peer_sockaddr() << " connected:" << (tcp.connected() ? "yes" : "no");
}

std::ostream& operator<< (std::ostream& os, const TCPConnectRequest& r) {
    if (r.sa_) return os << r.sa_;
    else       return os << r.host_ << ':' << r.service_;
}

}} // namespace panda::event
