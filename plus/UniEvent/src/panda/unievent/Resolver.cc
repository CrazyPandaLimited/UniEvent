#include "Resolver.h"

#include <algorithm>
#include <functional>

namespace panda { namespace unievent {

SimpleResolver::~SimpleResolver() {
    _EDTOR();
    ares_destroy(channel);
    timer->stop();
}

SimpleResolver::SimpleResolver(Loop* loop) : loop(loop) {
    _ECTOR();
    ares_options options;
    options.sock_state_cb      = ares_sockstate_cb;
    options.sock_state_cb_data = this;
    int optmask                = ARES_OPT_SOCK_STATE_CB | ARES_FLAG_NOALIASES;
    if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) {
        throw CodeError(ERRNO_RESOLVE);
    }

    timer = new Timer(loop);
    timer->timer_event.add([&](Timer*) {
        _EDEBUG("resolve request timed out");
        ares_process_fd(channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    });
}

void SimpleResolver::ares_sockstate_cb(void* data, sock_t sock, int read, int write) {
    SimpleResolverSP resolver = static_cast<SimpleResolver*>(data);
    auto task_pos = resolver->tasks.find(sock);
    AresTaskSP task = task_pos != std::end(resolver->tasks) ? task_pos->second : nullptr;
    
    _EDEBUG("resolver:%p task:%p sock:%d read:%d write:%d", resolver, task.get(), sock, read, write);

    if (read || write) {
        if (!task) {
            if (!resolver->timer->active()) {
                resolver->timer->start(DEFAULT_RESOLVE_TIMEOUT);
            }
            task = new AresTask(resolver->loop);
            resolver->tasks.emplace(sock, task);
        }

        task->start(sock, (read ? Poll::READABLE : 0) | (write ? Poll::WRITABLE : 0), [=](Poll* handle, int events, const CodeError* err) {
            resolver->timer->again();
            ares_process_fd(resolver->channel, sock, sock);
        });
    } else {
        // c-ares notifies us that the socket is closed
        assert(task && "No ares task");
        resolver->tasks.erase(task_pos);
        if (resolver->tasks.empty()) {
            resolver->timer->stop();
        }
    }
}

void SimpleResolver::resolve(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints, ResolveFunction callback) {
    ResolveRequestSP resolve_request(new ResolveRequest(callback));
    resolve_request->key      = iptr<cached_resolver::Key>(new cached_resolver::Key(string(node), string(service), hints->clone()));
    resolve_request->resolver = this;
    resolve_request->retain();
    retain();

    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    _EDEBUGTHIS("resolve resolve_request:%p {%s:%s}, loop:%p", resolve_request.get(), node_cstr, service_cstr, loop);
    ares_addrinfo h = hints->to<ares_addrinfo>();
    ares_getaddrinfo(channel, node_cstr, service_cstr, &h, ares_resolve_cb, resolve_request.get());
    resolve_request->async = true;
}

void SimpleResolver::ares_resolve_cb(void* arg, int status, int timeouts, ares_addrinfo* ai) {
    ResolveRequest* resolve_request = static_cast<ResolveRequest*>(arg);
    SimpleResolver*       resolver        = resolve_request->resolver;

    _EDEBUG("ares_resolve_cb {resolve_request:%p}{status:%d}", resolve_request, status);

    CodeError  err;
    AddrInfoSP addr;
    if (status == ARES_SUCCESS) {
        addr = new AddrInfo(ai);
    } else {
        err = CodeError(ERRNO_RESOLVE);
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

void SimpleResolver::on_resolve(SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err) {
    _EDEBUG("on_resolve {resolve_request:%p}{err:%d}", resolve_request.get(), err ? err->code() : 0);
    resolve_request->event(resolver, resolve_request, address, err);
}

Resolver::~Resolver() { _EDTOR(); }

Resolver::Resolver(Loop* loop, time_t expiration_time, size_t limit) : SimpleResolver(loop), expiration_time_(expiration_time), limit_(limit) {
    _ECTOR();
}

std::tuple<Resolver::CacheType::const_iterator, bool>
Resolver::find(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints) {
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

void Resolver::resolve(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints, ResolveFunction callback) {
    _EDEBUGTHIS("resolve");
    CacheType::const_iterator address_pos;
    bool                      found;
    std::tie(address_pos, found) = find(node, service, hints);
    if (found) {
        if (callback) {
            ResolveRequestSP resolve_request;
            callback(this, resolve_request, address_pos->second, nullptr);
        }
    }

    SimpleResolver::resolve(node, service, hints, callback);
}

void Resolver::on_resolve(SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err) {
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

}} // namespace panda::unievent
