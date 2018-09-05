#include <panda/unievent/CachedResolver.h>
#include <ctime>
#include <thread> // for std::thread::id

namespace panda { namespace unievent {

CachedResolver::~CachedResolver() { _ETRACETHIS("dtor, [thread:%zx]", std::hash<std::thread::id>()(std::this_thread::get_id())); }

CachedResolver::CachedResolver (time_t expiration_time, size_t limit) : expiration_time_(expiration_time), limit_(limit) {
    _ETRACETHIS("ctor, [thread:%zx]", std::hash<std::thread::id>()(std::this_thread::get_id()));
}

ResolveRequestSP CachedResolver::resolve_async (Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints, ResolveFunction callback) {
    ResolveRequestSP resolve_request(new ResolveRequest(callback));
    _pex_(resolve_request)->loop = _pex_(loop);
    resolve_request->key         = iptr<cached_resolver::Key>(new cached_resolver::Key(string(node), string(service), hints));
    resolve_request->resolver    = this;
    resolve_request->retain();

    _EDEBUGTHIS("resolve async, going async {resolve_request:%p}", resolve_request.get());
    int err = uv_getaddrinfo(_pex_(loop), _pex_(resolve_request), uvx_on_resolve, string(node).c_str(), service.length() ? string(service).c_str() : nullptr, hints);
    if (err) throw CodeError(err);

    resolve_request->retain();
    retain();

    return resolve_request;
}

ResolveRequestSP CachedResolver::resolve_async_compat (Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints, resolve_fn callback) {
    ResolveRequestSP resolve_request(new ResolveRequest(callback));
    _pex_(resolve_request)->loop = _pex_(loop);
    resolve_request->key         = iptr<cached_resolver::Key>(new cached_resolver::Key(string(node), string(service), hints));
    resolve_request->resolver    = this;
    resolve_request->retain();

    int err = uv_getaddrinfo(_pex_(loop), _pex_(resolve_request), uvx_on_resolve, string(node).c_str(), service.length() ? string(service).c_str() : nullptr, hints);
    if (err) throw CodeError(err);

    resolve_request->retain();
    retain();

    return resolve_request;
}

void CachedResolver::uvx_on_resolve (uv_getaddrinfo_t* req, int status, addrinfo* res) {
    auto            resolve_request = rcast<ResolveRequest*>(req);
    CachedResolver* resolver        = resolve_request->resolver;

    _EDEBUG("uvx_on_resolve {resolve_request:%p}{status:%d}{res:%p}{is_canceled:%d}", resolve_request, status, res, resolve_request->canceled());

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

    CodeError err(status);
    bool die = false;
    if (resolve_request->event.has_listeners())
        resolve_request->event(res, err, false);
    else if (resolve_request->event_compat.has_listeners())
        resolve_request->event_compat(resolver, res, err, false);
    else
        die = true;

    resolve_request->release();
    resolver->release();

    if (die) throw ImplRequiredError("CachedResolver::uvx_on_resolve, no listeners");
}

ResolveRequest::ResolveRequest (resolve_fn callback) : resolver(nullptr), key(nullptr) {
    _ETRACETHIS("ctor");
    event_compat.add(callback);
    _init(&uvr);
}

ResolveRequest::ResolveRequest (ResolveFunction callback) : resolver(nullptr), key(nullptr) {
    _ETRACETHIS("ctor");
    event.add(callback);
    _init(&uvr);
}

}} // namespace panda::event
