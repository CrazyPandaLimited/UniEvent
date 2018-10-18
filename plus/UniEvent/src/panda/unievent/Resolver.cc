#include "Resolver.h"

#include <algorithm>
#include <functional>

namespace panda { namespace unievent {

Resolver::~Resolver() {
    _EDTOR();
    ares_destroy(channel);
}

Resolver::Resolver(Loop* loop) : loop(loop) {
    _ECTOR();
    ares_options options;
    options.sock_state_cb      = ares_sockstate_cb;
    options.sock_state_cb_data = this;
    int optmask                = ARES_OPT_SOCK_STATE_CB;
    if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) {
        throw CodeError(ERRNO_RESOLVE);
    }

    timer = new Timer(loop);
    timer->timer_event.add([&](Timer*) {
        _EDEBUG("resolve request timed out");
        ares_process_fd(channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    });
}

void Resolver::ares_sockstate_cb(void* data, sock_t sock, int read, int write) {
    Resolver* resolver = static_cast<Resolver*>(data);
    auto pos = resolver->requests.find(sock);
    ResolveRequest* resolve_request = (pos == std::end(resolver->requests)) ? nullptr : *pos; 
    
    _EDEBUG("resolver:%p resolve_request:%p sock:%d read:%d write:%d", resolver, resolve_request, sock, read, write);

    if (read || write) {
        //if (!resolve_request->active()) {
            //if (!resolver->timer->active()) {
                //resolver->timer->start(DEFAULT_RESOLVE_TIMEOUT);
            //}
            //resolve_request->activate(sock);
        //}
    } else {
        // c-ares notifies us that the socket is closed
        assert(resolve_request);
        //resolve_request->close();
        //if (--resolver->request_counter == 0) {
            //resolver->timer->stop();
        //}
    }
}

ResolveRequestSP Resolver::resolve(Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints, ResolveFunction callback) {
    ResolveRequestSP resolve_request(new ResolveRequest(callback));
    resolve_request->key      = iptr<cached_resolver::Key>(new cached_resolver::Key(string(node), string(service), hints));
    resolve_request->resolver = this;
    resolve_request->retain();
    retain();

    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    _EDEBUGTHIS("resolve resolve_request:%p {%s:%s}, loop:%p", resolve_request.get(), node_cstr, service_cstr, loop);
    ares_gethostbyname(channel, node_cstr, AF_INET, ares_resolve_cb, 0);
    resolve_request->async = true;
    return resolve_request;
}

void Resolver::ares_resolve_cb(void* arg, int status, int timeouts, hostent* he) {
    ResolveRequest* resolve_request = static_cast<ResolveRequest*>(arg);
    Resolver*       resolver        = resolve_request->resolver;

    _EDEBUG("ares_resolve_cb {resolve_request:%p}{status:%d}", resolve_request, status);

    CodeError      err;
    BasicAddressSP addr;
    if (status != ARES_SUCCESS) {
        err = CodeError(ERRNO_RESOLVE);
    } else {
        in_addr addr;
        int     i = 0;
        while (he->h_addr_list[i] != 0) {
            addr.s_addr = *(u_long*)he->h_addr_list[i++];
            printf("\tIPv4 Address #%d: %s\n", i, inet_ntoa(addr));
        }

        // addr = new BasicAddress(to_addrinfo(res));
    }

    if (resolve_request->async) {
        resolver->on_resolve(resolver, resolve_request, addr, err);
        resolve_request->release();
        resolver->release();
    } else {
        TimerSP timer = new Timer(resolver->loop);
        timer->timer_event.add([=](Timer* timer) {
            timer->reset();
            resolver->on_resolve(resolver, resolve_request, addr, err);
            resolve_request->release();
            resolver->release();
        });
        timer->once(0);
    }
}

void Resolver::on_resolve(ResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) {
    _EDEBUG("on_resolve {resolve_request:%p}{err:%d}", resolve_request.get(), err ? err->code() : 0);
    resolve_request->event(resolver, resolve_request, address, err);
}

CachedResolver::~CachedResolver() { _EDTOR(); }

CachedResolver::CachedResolver(Loop* loop, time_t expiration_time, size_t limit) : Resolver(loop), expiration_time_(expiration_time), limit_(limit) {
    _ECTOR();
}

std::tuple<CachedResolver::CacheType::const_iterator, bool>
CachedResolver::find(std::string_view node, std::string_view service, const addrinfo* hints) {
    iptr<cached_resolver::Key> key(new cached_resolver::Key(string(node), string(service), hints));

    _EDEBUG("looking in cache [%.*s] [%.*s] %zd", (int)node.length(), node.data(), (int)service.length(), service.data(), cache_.size());

    auto address_pos = cache_.find(*key);
    if (address_pos != end(cache_)) {
        _EDEBUG("node in cache [%.*s]", (int)node.length(), node.data());

        time_t now = time(0);
        if (address_pos->second->expired(now, expiration_time_)) {
            cache_.erase(address_pos);
        } else {
            return std::make_tuple<CacheType::const_iterator, bool>(address_pos, true);
        }
    }

    return std::make_tuple<CacheType::const_iterator, bool>(address_pos, false);
}

ResolveRequestSP
CachedResolver::resolve(Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints, ResolveFunction callback) {
    _EDEBUGTHIS("resolve");
    CacheType::const_iterator address_pos;
    bool                      found;
    std::tie(address_pos, found) = find(node, service, hints);
    if (found) {
        if (callback) {
            ResolveRequestSP resolve_request;
            callback(this, resolve_request, address_pos->second, nullptr);
            return resolve_request;
        }
    }

    return Resolver::resolve(loop, node, service, hints, callback);
}

void CachedResolver::on_resolve(ResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) {
    _EDEBUG("on_resolve {resolve_request:%p}{err:%d}", resolve_request.get(), err ? err->code() : 0);
    if (!err) {
        expunge_cache();
        // use address from the cache
        address = cache_.emplace(*resolve_request->key, CachedAddressSP(new CachedAddress(address))).first->second;
    }
    resolve_request->event(resolver, resolve_request, address, err);
}
ResolveRequest::~ResolveRequest() { _EDTOR(); }

ResolveRequest::ResolveRequest(ResolveFunction callback) : resolver(nullptr), key(nullptr), async(false) {
    _ECTOR();
    event.add(callback);
}

void ResolveRequest::activate(sock_t sock) {
    poll    = PollSP(new Poll(-1, sock, resolver->loop));
    active_ = true;
}

void ResolveRequest::close() {
    active_ = false;
    poll->stop();
}

}} // namespace panda::unievent
