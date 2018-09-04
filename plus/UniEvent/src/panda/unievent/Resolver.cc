#include <panda/unievent/Resolver.h>

namespace panda { namespace unievent {

Resolver::~Resolver() {
    _ETRACETHIS("dtor");
}

Resolver::Resolver(Loop* loop) {
    _ETRACETHIS("ctor");
    uvr.loop = _pex_(loop);
    _init(&uvr);
}

void Resolver::resolve_compat (std::string_view node, std::string_view service, const addrinfo* hints, resolve_fn callback) {
    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);
    
    if (callback) {
        resolve_event_compat.add(callback);
    }

    int err = uv_getaddrinfo(uvr.loop, &uvr, uvx_on_resolve, node_cstr, service.length() ? service_cstr : nullptr, hints);
    if (err) { 
        throw CodeError(err);
    }

    retain();
}

void Resolver::resolve (std::string_view node, std::string_view service, const addrinfo* hints, ResolveFunction callback) {
    _EDEBUG("[%p] resolve", this);

    PEXS_NULL_TERMINATE(node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    if (callback)
        resolve_event.add(callback);

    int err = uv_getaddrinfo(uvr.loop, &uvr, uvx_on_resolve, node_cstr, service.length() ? service_cstr : nullptr, hints);
    if (err)
        throw CodeError(err);

    retain();
}


void Resolver::free (addrinfo* ai) {
    uv_freeaddrinfo(ai); 
}

void Resolver::call_on_resolve (addrinfo* res, const CodeError* err) {
    if (canceled_) {
        CodeError err(UV_ECANCELED);
        on_resolve(res, &err);
    } else {
        on_resolve(res, err);
    }
}

void Resolver::on_resolve (addrinfo* res, const CodeError* err) {
    if (resolve_event.has_listeners())
        resolve_event(res, err, false);
    else if (resolve_event_compat.has_listeners())
        resolve_event_compat(this, res, err);
    else
        throw ImplRequiredError("AddrInfo::on_resolve");
}

void Resolver::uvx_on_resolve (uv_getaddrinfo_t* req, int status, addrinfo* res) {
    Resolver* r = rcast<Resolver*>(req);
    _EDEBUG("uvx_on_resolve resolver:%p resolvercnt:%d status:%d", r, r->refcnt(), status);
    CodeError err(status);
    r->call_on_resolve(res, &err);
    r->release();
}

}}

