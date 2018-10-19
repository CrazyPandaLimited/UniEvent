#include "TCP.h"

#include "Resolver.h"
#include "Prepare.h"
#include "ssl/SSLFilter.h"
#include "socks/SocksFilter.h"

namespace panda { namespace unievent {

namespace {

addrinfo init_default_hints() {
    addrinfo ret;
    memset(&ret, 0, sizeof(ret));
    ret.ai_family   = PF_UNSPEC;
    ret.ai_socktype = SOCK_STREAM;
    ret.ai_flags    = AI_PASSIVE;
    return ret;
}

uint16_t getenv_free_port() {
    const char*      env     = getenv("UNIEVENT_FREE_PORT");
    static uint16_t env_int = env ? atoi(env) : 0;
    return env_int;
}

uint16_t any_free_port(Loop* loop) {
    iptr<TCP> test = new TCP(loop);
    test->bind("localhost", "0");
    auto sa = test->get_sockaddr();
    auto port = sa.is_inet4() ? sa.inet4().port() : sa.inet6().port();
    test.reset();
    loop->run_nowait();
    if (port > 1024) {
        return port;
    } else {
        return any_free_port(loop);
    }
}

bool check_if_free(uint16_t port, Loop* loop) {
    iptr<TCP> test = new TCP(loop);
    try {
        test->bind("localhost", panda::to_string(port));
        test.reset();
        loop->run_nowait();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

addrinfo TCP::defhints = init_default_hints();

uint16_t find_free_port() {
    thread_local iptr<Loop> loop(new Loop);
    static uint16_t port = getenv_free_port();
    if (port) {
        while (!check_if_free(port++, loop)) {
        }
        return port;
    } else {
        return any_free_port(loop);
    }
}

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
        resolver = get_thread_local_cached_resolver(loop());
    } else {
        resolver = get_thread_local_simple_resolver(loop());
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

void TCP::connect_internal(TCPConnectRequest* tcp_connect_request) {
    if (!tcp_connect_request->sa_) {
        _EDEBUGTHIS("connect_internal, resolving %p", tcp_connect_request);
        try {
            resolve_request =
                resolver->resolve(loop(), tcp_connect_request->host_, tcp_connect_request->service_, &tcp_connect_request->hints_,
                                  [=](ResolverSP, ResolveRequestSP, BasicAddressSP address, const CodeError* err) {
                                      _EDEBUG("resolve callback, err: %d", err ? err->code() : 0);
                                      if (err) {
                                          int errcode = err->code();
                                          Prepare::call_soon([=] { call_filters(&StreamFilter::on_connect, CodeError(errcode), tcp_connect_request); }, loop());
                                          return;
                                      }

                                      tcp_connect_request->sa_ = address->head->ai_addr;

                                      connect_internal(tcp_connect_request);
                                  });
            return;
        } catch (...) {
            Prepare::call_soon([=] { call_filters(&StreamFilter::on_connect, CodeError(ERRNO_RESOLVE), tcp_connect_request); }, loop());
            return;
        }
    }
    
    int err = uv_tcp_connect(_pex_(tcp_connect_request), &uvh, tcp_connect_request->sa_.get(), Stream::uvx_on_connect);
    if (err) {
        Prepare::call_soon([=] { call_filters(&StreamFilter::on_connect, CodeError(err), tcp_connect_request); }, loop());
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

    call_filters(&StreamFilter::connect, tcp_connect_request);    
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

// use_socks and use_ssl will work with two filters only
// for custom configuration add filters manually
void TCP::use_ssl (const SSL_METHOD* method) {
    if (is_secure()) {
        return;
    }
    
    if(!method && listening()) {
        throw Error("Programming error, use server certificate");
    }

    auto pos = find_filter<socks::SocksFilter>();
    if(pos == filters_.end()) {
        // insert right after default front filter if there are no other filters
        filters_.insert(++filters_.begin(), new ssl::SSLFilter(this, method));
    } else {
        // insert before socks as socks has its own auth encryption methods
        filters_.insert(pos, new ssl::SSLFilter(this, method));
    }
}

void TCP::use_socks(std::string_view host, uint16_t port, std::string_view login, std::string_view passw, bool socks_resolve) {
    use_socks(new Socks(string(host), port, string(login), string(passw), socks_resolve));
}

void TCP::use_socks(const SocksSP& socks) { 
    auto pos = find_filter<socks::SocksFilter>();
    if(pos == filters_.end()) {
        // always insert socks as last (before default back) filter
        filters_.insert(--filters_.end(), new socks::SocksFilter(this, socks));
    } else {
        filters_.insert(filters_.erase(pos), new socks::SocksFilter(this, socks));
    }
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
