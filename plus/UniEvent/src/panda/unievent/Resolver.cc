#include "Timer.h"
#include "Prepare.h"
#include "Resolver.h"
#include <functional>

namespace panda { namespace unievent {

ResolveRequestSP Resolver::resolve(Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints, ResolveFunction callback) {
    ResolveRequestSP resolve_request(new ResolveRequest(callback));
    resolve_request->key         = iptr<cached_resolver::Key>(new cached_resolver::Key(string(node), string(service), hints));
    resolve_request->resolver    = this;
    resolve_request->retain();
    retain();

    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    _EDEBUGTHIS("resolve resolve_request:%p {%s:%s}, loop:%p", resolve_request.get(), node_cstr, service_cstr, loop);
    addrinfo* res;
    int syserr = getaddrinfo(node_cstr, service_cstr, hints, &res);
    TimerSP t = new Timer(loop);
    t->timer_event.add([=](TimerSP) mutable {
        t.reset(); // just to capture timer and prevent it from death
        _EDEBUG("on_resolve {resolve_request:%p}{syserr:%d}{res:%p}{is_canceled:%d}", resolve_request, syserr, res, resolve_request->canceled());
        CodeError err;
        BasicAddressSP addr;

        if (syserr) {
            err = CodeError(_err_gai2uv(syserr));
        } else if (resolve_request->canceled()) {
            err = CodeError(ERRNO_ECANCELED);
        } else {
            addr = new BasicAddress(res);
        }

        this->on_resolve(this, resolve_request, addr, err);

        resolve_request->release();
        this->release();
    });
    t->once(0);

    return resolve_request;
}

void Resolver::on_resolve(AbstractResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) {
    _EDEBUG("on_resolve {resolve_request:%p}{err:%d}{is_canceled:%d}", resolve_request.get(), err ? err->code() : 0, resolve_request->canceled());
    resolve_request->event(resolver, resolve_request, address, err);
}

CachedResolver::~CachedResolver() { _EDTOR(); }

CachedResolver::CachedResolver(time_t expiration_time, size_t limit) : expiration_time_(expiration_time), limit_(limit) { _ECTOR(); }

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

void CachedResolver::on_resolve(AbstractResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) {
    _EDEBUG("on_resolve {resolve_request:%p}{err:%d}{is_canceled:%d}", resolve_request.get(), err ? err->code() : 0, resolve_request->canceled());
    if (!err) {
        expunge_cache();
        // use address from the cache
        address = cache_.emplace(*resolve_request->key, CachedAddressSP(new CachedAddress(address))).first->second;
    }
    resolve_request->event(resolver, resolve_request, address, err);
}

ResolveRequest::ResolveRequest(ResolveFunction callback) : resolver(nullptr), key(nullptr) {
    _ECTOR();
    event.add(callback);
}

void ResolveRequest::cancel() {
    if (this->canceled_) return;

    canceled_ = true;
}

}}
