#include <panda/unievent/CachedResolver.h>

#include <ctime>
#include <thread> // for std::thread::id

namespace panda { namespace unievent {

CachedResolver::~CachedResolver() { _ETRACETHIS("dtor, [thread:%zx]", std::hash<std::thread::id>()(std::this_thread::get_id())); }

CachedResolver::CachedResolver(time_t expiration_time, size_t limit)
        : expiration_time_(expiration_time)
        , limit_(limit) {
    _ETRACETHIS("ctor, [thread:%zx]", std::hash<std::thread::id>()(std::this_thread::get_id()));
}

iptr<ResolveRequest> CachedResolver::resolve_async(Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints,
                                                   ResolveFunction callback) {
    iptr<ResolveRequest> resolve_request(new ResolveRequest(callback));
    _pex_(resolve_request)->loop = _pex_(loop);
    resolve_request->key         = iptr<cached_resolver::Key>(new cached_resolver::Key(string(node), string(service), hints));
    resolve_request->resolver    = this;
    resolve_request->retain();

    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    _EDEBUGTHIS("resolve async, going async {resolve_request:%p}", resolve_request.get());
    int err = uv_getaddrinfo(_pex_(loop), _pex_(resolve_request), uvx_on_resolve, node_cstr, service.length() ? service_cstr : nullptr, hints);
    if (err) {
        _EDEBUGTHIS("uv_getaddinfo failed %d", err);
        resolve_request->release();
        throw ResolveError(err);
    }

    retain();

    return resolve_request;
}

iptr<ResolveRequest> CachedResolver::resolve_async_compat(Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints,
                                                          resolve_fn callback) {
    iptr<ResolveRequest> resolve_request(new ResolveRequest(callback));
    _pex_(resolve_request)->loop = _pex_(loop);
    resolve_request->key         = iptr<cached_resolver::Key>(new cached_resolver::Key(string(node), string(service), hints));
    resolve_request->resolver    = this;
    resolve_request->retain();
    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    int err = uv_getaddrinfo(_pex_(loop), _pex_(resolve_request), uvx_on_resolve, node_cstr, service.length() ? service_cstr : nullptr, hints);
    if (err) {
        resolve_request->release();
        throw ResolveError(err);
    }

    retain();

    return resolve_request;
}

void CachedResolver::uvx_on_resolve(uv_getaddrinfo_t* req, int status, addrinfo* res) {
    iptr<ResolveRequest> resolve_request = rcast<ResolveRequest*>(req);
    CachedResolver*      resolver        = resolve_request->resolver;

    _EDEBUG("uvx_on_resolve {resolve_request:%p}{status:%d}{res:%p}{is_canceled:%d}", resolve_request.get(), status, res, resolve_request->canceled());

    if (resolve_request->canceled()) {
        status = UV_ECANCELED;
    } else if (!status) {
        resolver->expunge_cache();

        auto address_pos =
            resolver->cache_.emplace(*resolve_request->key, iptr<cached_resolver::Address>(new cached_resolver::Address(res, time(0)))).first;

        // use res from the cache
        res = address_pos->second->head;

        _EDEBUG("uvx resolve, res:%p cache_size:%zd", res, resolver->cache_.size());
    }

    if (resolve_request->event.has_listeners()) {
        resolve_request->event(res, ResolveError(status), false);
    } else if (resolve_request->event_compat.has_listeners()) {
        resolve_request->event_compat(resolver, res, ResolveError(status), false);
    } else {
        resolve_request->release();
        resolver->release();
        throw ImplRequiredError("CachedResolver::uvx_on_resolve, no listeners");
    }

    resolve_request->release();
    resolver->release();
}

ResolveRequest::ResolveRequest(resolve_fn callback)
        : resolver(nullptr)
        , key(nullptr) {
    _ETRACETHIS("ctor");
    event_compat.add(callback);
    _init(&uvr);
}

ResolveRequest::ResolveRequest(ResolveFunction callback)
        : resolver(nullptr)
        , key(nullptr) {
    _ETRACETHIS("ctor");
    event.add(callback);
    _init(&uvr);
}

}} // namespace panda::event
