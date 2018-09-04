#include <panda/unievent/Resolver.h>
#include <panda/unievent/TCP.h>
#include <panda/unievent/socks/SocksProxy.h>

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
/* SOCKS
bool parse_bool_parameter(const char* value) { return value && (strcmp(value, "1") == 0 || strcmp(value, "true")); }
*/
in_port_t getenv_free_port() {
    const char*      env     = getenv("PANDA_EVENT_FREE_PORT");
    static in_port_t env_int = env ? atoi(env) : 0;
    return env_int;
}
/* SOCKS
iptr<Socks> getenv_proxy() {
    // PANDA_EVENT_PROXY="socks5://[user:password@]proxyhost[:port]"
    const char* env_proxy = getenv("PANDA_EVENT_PROXY");
    // PANDA_EVENT_PROXY_RESOLVE="1"
    const char* env_proxy_resolve = getenv("PANDA_EVENT_PROXY_RESOLVE");

    static iptr<Socks> socks;
    if (env_proxy) {
        socks = new Socks(string(env_proxy), parse_bool_parameter(env_proxy_resolve));
    }
    return socks;
}
*/
// get port, IPv4 or IPv6:
in_port_t get_in_port(sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return (((sockaddr_in*)sa)->sin_port);
    }
    return (((sockaddr_in6*)sa)->sin6_port);
}

in_port_t any_free_port(Loop* loop) {
    iptr<TCP> test = new TCP(loop);
    test->bind("localhost", "0");

    int              namelen = sizeof(sockaddr_storage);
    sockaddr_storage sas;
    memset(&sas, 0, sizeof(sas));
    sockaddr* res = (sockaddr*)&sas;
    test->getsockname(res, &namelen);
    in_port_t port = get_in_port(res);
    test.reset();
    loop->run_nowait();
    if (port > 1024) {
        return port;
    } else {
        return any_free_port(loop);
    }
}

bool check_if_free(in_port_t port, Loop* loop) {
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

in_port_t find_free_port() {
    thread_local iptr<Loop> loop(new Loop);
    static in_port_t        port = getenv_free_port();
    if (port) {
        while (!check_if_free(port++, loop)) {
        }
        return port;
    } else {
        return any_free_port(loop);
    }
}

TCP::~TCP() { _ETRACETHIS("dtor"); }

TCP::TCP(Loop* loop) {
    _EDEBUGTHIS("ctor");
    int err = uv_tcp_init(_pex_(loop), &uvh);
    if (err)
        throw CodeError(err);
    _init(&uvh);
/* SOCKS
    static iptr<Socks> socks = getenv_proxy();
    if (socks) {
        use_socks(socks);
    }
*/
}

TCP::TCP(Loop* loop, unsigned int flags) {
    _EDEBUGTHIS("ctor");
    int err = uv_tcp_init_ex(_pex_(loop), &uvh, flags);
    if (err)
        throw CodeError(err);
    _init(&uvh);
/* SOCKS
    static iptr<Socks> socks = getenv_proxy();
    if (socks) {
        use_socks(socks);
    }
*/
}

void TCP::open(sock_t socket) {
    int err = uv_tcp_open(&uvh, socket);
    if (err)
        throw CodeError(err);
}

void TCP::bind(const sockaddr* sa, unsigned int flags) {
    int err = uv_tcp_bind(&uvh, sa, flags);
    if (err)
        throw CodeError(err);
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

// @returns true if async is needed
bool TCP::resolve (const string& host, const string& service, const addrinfo* hints, ResolveFunction callback, bool use_cached_resolver) {
    _EDEBUGTHIS("resolve %.*s:%.*s", (int)host.length(), host.data(), (int)service.length(), service.data());

    if (use_cached_resolver) {
        if (!resolve_with_cached_resolver(host, service, hints, callback)) {
            return false;
        }
    } else {
        if (!resolve_with_regular_resolver(host, service, hints, callback)) {
            return false;
        }
    }

    _EDEBUGTHIS("resolve going async");

    async_lock();
    retain();

    return true;
}

// @returns true if async is needed
bool TCP::resolve_with_cached_resolver (const string& host, const string& service, const addrinfo* hints, ResolveFunction callback) {
    CachedResolver*                           resolver = get_thread_local_cached_resolver();
    CachedResolver::CacheType::const_iterator address_pos;
    bool                                      found;
    std::tie(address_pos, found) = resolver->find(host, service, hints);
    if (found) {
        callback(address_pos->second->next(), nullptr, true);
        return false;
    }

    if (async_locked()) {
        _EDEBUGTHIS("locked, queuing");
        asyncq_push(new CommandResolveHost(this, host, service, hints, callback, true));
        return false;
    }

    try {
        resolve_request = resolver->resolve_async(loop(), host, service, hints, [=](addrinfo* res, const CodeError* err, bool) {
            if (err) {
                callback(nullptr, err, false);
                return;
            }

            try {
                // unlock for a moment, h->connect will lock it again, otherwise it would
                // enqueue and wait for nothing
                this->async_unlock_noresume();
                callback(res, nullptr, false);
            } catch (const CodeError& err) {
                // sync error while connecting - no other ways but call as async
                // connect did not lock it again
                this->async_lock();
                callback(nullptr, &err, false);
            }
            // h->connect called retain() again
            this->release();
        });
    } catch (...) {
        _EDEBUGTHIS("thrown");
        throw;
    }

    return true;
}

// @returns true if async is needed
bool TCP::resolve_with_regular_resolver (const string& host, const string& service, const addrinfo* hints, ResolveFunction callback) {
    if (async_locked()) {
        _EDEBUGTHIS("locked, queuing");
        asyncq_push(new CommandResolveHost(this, host, service, hints, callback, false));
        return false;
    }

    resolver = iptr<Resolver>(new Resolver(loop()));

    resolver->handle = this;
    resolver->retain();

    try {
        resolver->resolve(host, service, hints, [=](addrinfo* res, const CodeError* err, bool) {
            _EDEBUGTHIS("resolve wrapped callback");
            resolver->release();

            if (err) {
                callback(nullptr, err, false);
                return;
            }

            try {
                // unlock for a moment, h->connect will lock it again, otherwise it would enqueue and wait for nothing
                this->async_unlock_noresume();
                callback(res, nullptr, false);
                Resolver::free(res);
            } catch (const CodeError& err) {
                // sync error while connecting - no other ways but call as async
                // connect did not lock it again
                this->async_lock();
                callback(nullptr, &err, false);
                Resolver::free(res);
            }
            // h->connect called retain() again
            this->release();
        });

        return true;
    } catch (...) {
        _EDEBUGTHIS("thrown");
        resolver->release();
        throw;
    }
}

void TCP::connect (const string& host, const string& service, const addrinfo* hints, ConnectRequest* connect_request, bool use_cached_resolver) {
    _EDEBUGTHIS("connect to %.*s:%.*s", (int)host.length(), host.data(), (int)service.length(), service.data());

    if (!connect_request) {
        connect_request = new ConnectRequest();
    }

    _pex_(connect_request)->handle = _pex_(this);

    if (async_locked()) {
        _EDEBUGTHIS("locked, queuing");
        asyncq_push(new CommandConnectHost(this, host, service, hints, connect_request, use_cached_resolver));
        return;
    }

    connect_request->retain();
/* SOCKS
    if (use_socks()) {
        if (socks_proxy->start(host, service, hints, connect_request)) {
            _EDEBUGTHIS("start proxying");
            flags |= SF_CONNECTING;
            async_lock();
            retain();
            return;
        } else {
            _EDEBUGTHIS("proxying failed");
            connect_request->release();
            throw CodeError(ERRNO_SOCKS);
        }
    }
*/
    try {
        if (!resolve(host, service, hints,
                     [=](addrinfo* res, const CodeError* err, bool) {
                         _EDEBUG("resolve callback");
                         if (err) {
                             Stream::uvx_on_connect(_pex_(connect_request), (int)err->code());
                             return;
                         }
                         connect(res->ai_addr, connect_request);
                         connect_request->release();
                     },
                     use_cached_resolver)) {
            // it means that async is locked or connect() has been called
            return;
        }
    } catch (...) {
        _EDEBUGTHIS("thrown");
        connect_request->release();
        throw;
    }

    _EDEBUGTHIS("connect going async");

    flags |= SF_CONNECTING;
}

void TCP::connect (const sockaddr* sa, ConnectRequest* connect_request) {
    _EDEBUGTHIS("connect to sock:%p, connect_request:%p", sa, connect_request);

    if (!connect_request) connect_request = new ConnectRequest();

    _pex_(connect_request)->handle = _pex_(this);

    if (async_locked()) {
        _EDEBUGTHIS("locked, queuing");
        asyncq_push(new CommandConnectSockaddr(this, sa, connect_request));
        return;
    }

    connect_request->retain();
    /* SOCKS
    if (use_socks()) {
        if (socks_proxy->start(sa, connect_request)) {
            _EDEBUGTHIS("start proxying");
            flags |= SF_CONNECTING;
            async_lock();
            retain();
            return;
        } else {
            _EDEBUGTHIS("proxying failed");
            connect_request->release();
            throw CodeError(ERRNO_SOCKS);
        }
    }
    */
    _EDEBUGTHIS("connecting %p", connect_request);

    flags |= SF_CONNECTING;
    async_lock();
    retain();

    int err = uv_tcp_connect(_pex_(connect_request), &uvh, sa, Stream::uvx_on_connect);
    if (err) {
        CodeError err(err);
        call_on_connect(&err, connect_request);
        return;
    }
}

void TCP::reconnect (const sockaddr* sa, ConnectRequest* connect_request) {
    disconnect();
    if (!connect_request) connect_request = new ConnectRequest();

    connect_request->is_reconnect = true;
    connect(sa, connect_request);
}

void TCP::reconnect (const string& host, const string& service, const addrinfo* hints, ConnectRequest* connect_request, bool use_cached_resolver) {
    disconnect();
    if (!connect_request) connect_request = new ConnectRequest();

    connect_request->is_reconnect = true;
    connect(host, service, hints, connect_request, use_cached_resolver);
}
/* SOCKS
void TCP::use_socks(std::string_view host, uint16_t port, std::string_view login, std::string_view passw, bool socks_resolve) {
    use_socks(new Socks(string(host), port, string(login), string(passw), socks_resolve));
}

void TCP::use_socks(SocksSP socks) { socks_proxy = iptr<socks::SocksProxy>(new socks::SocksProxy(this, socks)); }
*/
void TCP::on_handle_reinit () {
    int err = uv_tcp_init(uvh.loop, &uvh);
    if (err) throw CodeError(err);
    Stream::on_handle_reinit();
}

void TCP::close_reinit (bool keep_asyncq) {
    _EDEBUGTHIS("close_reinit, keep_async: %d", keep_asyncq);
    if (!keep_asyncq && resolver) {
        resolver->cancel();
    }
    if (!keep_asyncq && resolve_request) {
        resolve_request->cancel();
    }
    /* SOCKS
    if (!keep_asyncq && socks_proxy) {
        socks_proxy->cancel();
    }
    */
    Handle::close_reinit(keep_asyncq);
}

void TCP::_close() {
    //_EDEBUGTHIS("close resolver:%p, resolve_request:%p socks_proxy:%p", resolver, resolve_request, socks_proxy);
    if (resolver) {
        resolver->cancel();
    }
    if (resolve_request) {
        resolve_request->cancel();
    }
    /* SOCKS
    if (socks_proxy) {
        socks_proxy->cancel();
    }
    */
    Handle::_close();
}

TCP::ConnectBuilder::~ConnectBuilder() noexcept(false) {
    if (host_ && service_ && sa_) {
        throw Error("TCP::connect expect {host,port} OR sa, not BOTH");
    }

    if (timeout_) {
        if (!connect_request_) {
            connect_request_ = new ConnectRequest();
        }

        TCP*             tcp_capture             = tcp_;
        ConnectRequestSP connect_request_capture = connect_request_;

        // you can't capture 'this', as it will be dead when called
        connect_request_->set_timer(Timer::once(timeout_,
                                                [tcp_capture, connect_request_capture](Timer* t) {
                                                    _EDEBUG("connect timed out %p %p %u", t, tcp_capture, connect_request_capture->refcnt());
                                                    CodeError err(UV_ETIMEDOUT);
                                                    connect_request_capture->error = err;
                                                    connect_request_capture->release_timer();
                                                    tcp_capture->cancel_connect();
                                                },
                                                tcp_->loop()));
    }

    if (host_ && service_) {
        tcp_->connect(host_, service_, info_, connect_request_, cached_resolver_);
    } else if (sa_) {
        tcp_->connect(sa_, connect_request_.get());
    } else {
        throw Error("TCP::connect expect to(host,port) or to(sa) to be set");
    }
}

TCP::ReconnectBuilder::~ReconnectBuilder() noexcept(false) {
    tcp_->disconnect();
    if (!connect_request_) {
        connect_request_ = new ConnectRequest();
    }
    connect_request_->is_reconnect = true;
}

}} // namespace panda::event
