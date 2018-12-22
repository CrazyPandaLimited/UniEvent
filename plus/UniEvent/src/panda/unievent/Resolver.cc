#include "Timer.h"
#include "Prepare.h"
#include "Resolver.h"

#include <algorithm>
#include <functional>

#include <panda/net/sockaddr.h>

namespace panda { namespace unievent {

std::string AddrInfo::to_string() {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const AddrInfo& ai) {
    ares_addrinfo* next = ai.head;
    while (next) {
        os << panda::net::SockAddr(next->ai_addr);
        next = next->ai_next;
        if(next) {
            os << " ";
        }
    }
    return os;
}

SimpleResolver::~SimpleResolver() {
    _EDTOR();
    if (timer) {
        timer->stop();
    }
    tasks.clear();
    ares_destroy(channel);
}

SimpleResolver::SimpleResolver(Loop* loop) : loop(loop) {
    _ECTOR();
    ares_options options;
    int optmask = 0;

    options.sock_state_cb      = ares_sockstate_cb;
    options.sock_state_cb_data = this;
    optmask |= ARES_OPT_SOCK_STATE_CB;

    options.flags = ARES_FLAG_NOALIASES;
    optmask |= ARES_OPT_FLAGS;

    if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) {
        throw CodeError(ERRNO_RESOLVE);
    }

    timer = new Timer(loop);
    timer->timer_event.add([=](Timer*) {
        _EDEBUG("resolve request timed out");
        ares_process_fd(channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    });
}

void SimpleResolver::ares_sockstate_cb(void* data, sock_t sock, int read, int write) {
    SimpleResolverSP resolver = static_cast<SimpleResolver*>(data);
    auto task_pos = resolver->tasks.find(sock);
    AresTaskSP task = task_pos != std::end(resolver->tasks) ? task_pos->second : nullptr;
    
    _EDEBUG("resolver:%p task:%p sock:%d read:%d write:%d", resolver.get(), task.get(), sock, read, write);

    if (read || write) {
        if (!task) {
            if (!resolver->timer->active()) {
                resolver->timer->start(DEFAULT_RESOLVE_TIMEOUT);
            }
            task = new AresTask(resolver->loop);
            resolver->tasks.emplace(sock, task);
        }

        task->start(sock, (read ? Poll::READABLE : 0) | (write ? Poll::WRITABLE : 0), [=](Poll*, int, const CodeError*) {
            resolver->timer->again();
            ares_process_fd(resolver->channel, sock, sock);
        });
    } else {
        // c-ares notifies us that the socket is closed
        if(task) {
            resolver->tasks.erase(task_pos);
        }
        if (resolver->tasks.empty() && resolver->timer) {
            resolver->timer->stop();
        }
    }
}

ResolveRequestSP SimpleResolver::resolve(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints, ResolveFunction callback, bool use_cache) {
    ResolveRequestSP resolve_request(new ResolveRequest(callback, this));
    if(use_cache) {
        resolve_request->key = new ResolverCacheKey(string(node), string(service), hints->clone());
    }
    resolve_request->retain();
    retain();

    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    _EDEBUGTHIS("resolve_request:%p [%s:%s] loop:%p use_cache:%d", resolve_request.get(), node_cstr, service_cstr, loop, use_cache);
    ares_addrinfo h = hints->to<ares_addrinfo>();
    ares_getaddrinfo(channel, node_cstr, service_cstr, &h, ares_resolve_cb, resolve_request.get());
    resolve_request->async = true;
    return resolve_request;
}

void SimpleResolver::ares_resolve_cb(void* arg, int status, int, ares_addrinfo* ai) {
    ResolveRequest* resolve_request = static_cast<ResolveRequest*>(arg);
    SimpleResolver* resolver        = resolve_request->resolver;

    _EDEBUG("resolve_request:%p status:%d async:%d ai:%p", resolve_request, status, resolve_request->async, ai);

    CodeError  err;
    AddrInfoSP addr;
    if (status == ARES_SUCCESS) {
        addr = new AddrInfo(ai);
        #if EVENT_LIB_DEBUG >= 1
        std::string addr_str = addr->to_string();
        _EDEBUG("addr: %.*s", (int)addr_str.length(), addr_str.data());
        #endif
    } else {
        err = CodeError(ERRNO_RESOLVE);
    }

    if (resolve_request->async) {
        resolver->on_resolve(resolver, resolve_request, addr, err);
        resolve_request->release();
        resolver->release();
    } else {
        _EDEBUG();
        TimerSP t = new Timer(resolver->loop);
        t->timer_event.add([=](TimerSP) mutable {
            _EDEBUG("artificial async call");
            resolver->on_resolve(resolver, resolve_request, addr, err);
            resolve_request->release();
            resolver->release();
            t.reset();
        });
        t->once(0);
    }
}

void SimpleResolver::on_resolve(SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err) {
    _EDEBUGTHIS("resolve_request:%p err:%d", resolve_request.get(), err ? err->code() : 0);
    resolve_request->event(resolver, resolve_request, address, err);
}

Resolver::~Resolver() { _EDTOR(); }

Resolver::Resolver(Loop* loop, time_t expiration_time, size_t limit) : SimpleResolver(loop), expiration_time_(expiration_time), limit_(limit) {
    _ECTOR();
}

void SimpleResolver::stop() {
    _EDEBUGTHIS("");
    if (timer) {
        timer->stop();
        timer.reset();
    }
    tasks.clear();
    ares_cancel(channel);
}

std::tuple<ResolverCacheType::const_iterator, bool>
Resolver::find(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints) {
    _EDEBUGTHIS("looking in cache [%.*s:%.*s] cache_size: %zd", (int)node.length(), node.data(), (int)service.length(), service.data(), cache_.size());
    auto address_pos = cache_.find({string(node), string(service), hints});
    if (address_pos != end(cache_)) {
        _EDEBUGTHIS("found in cache [%.*s]", (int)node.length(), node.data());

        time_t now = time(0);
        if (address_pos->second.expired(now, expiration_time_)) {
            _EDEBUGTHIS("expired [%.*s]", (int)node.length(), node.data());
            cache_.erase(address_pos);
        } else {
            return std::make_tuple<ResolverCacheType::const_iterator, bool>(address_pos, true);
        }
    }

    return std::make_tuple<ResolverCacheType::const_iterator, bool>(address_pos, false);
}

ResolveRequestSP Resolver::resolve(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints, ResolveFunction callback, bool use_cache) {
    _EDEBUGTHIS("use_cache: %d", use_cache);
    if(use_cache) {
        ResolverCacheType::const_iterator address_pos;
        bool                              found;
        std::tie(address_pos, found) = find(node, service, hints);
        if (found) {
            ResolveRequestSP resolve_request = new ResolveRequest(callback, this);
            on_resolve(this, resolve_request, address_pos->second.address);
            return resolve_request;
        }
    }

    return SimpleResolver::resolve(node, service, hints, callback, use_cache);
}

void Resolver::on_resolve(SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err) {
    _EDEBUGTHIS("resolve_request:%p err:%d use_cache:%d", resolve_request.get(), err ? err->code() : 0, (bool)resolve_request->key);
    expunge_cache();
    if (!err && resolve_request->key) {
        // use address from the cache
        address = cache_.emplace(*resolve_request->key, CachedAddress{address}).first->second.address;
    }

    resolve_request->event(resolver, resolve_request, address, resolve_request->canceled ? CodeError(ERRNO_ECANCELED) : err);
}

ResolveRequest::~ResolveRequest() { _EDTOR(); }

ResolveRequest::ResolveRequest(ResolveFunction callback, SimpleResolver* resolver) : resolver(resolver), key(nullptr), async(false), canceled(false) {
    _ECTOR();
    event.add(callback);
}

}} // namespace panda::unievent
