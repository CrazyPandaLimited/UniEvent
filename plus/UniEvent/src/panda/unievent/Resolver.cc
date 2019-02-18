#include "Resolver.h"
#include "Timer.h"
#include "Prepare.h"
#include <algorithm>
#include <functional>

namespace panda { namespace unievent {

static constexpr const int DNS_ROLL_TIMEOUT = 1000; // [ms]

#define check_alive() { if (destroyed) throw Error("Loop has been destroyed and this resolver can not be used anymore"); }

bool AddrInfo::operator== (const AddrInfo& oth) const {
    if (cur == oth.cur) return true;
    return cur && oth.cur &&
           family()   == oth.family()   &&
           socktype() == oth.socktype() &&
           protocol() == oth.protocol() &&
           flags()    == oth.flags()    &&
           addr()     == oth.addr();
}

std::string AddrInfo::to_string () {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream& operator<< (std::ostream& os, const AddrInfo& ai) {
    auto cur = ai;
    while (cur) {
        os << cur.addr();
        cur = cur.next();
        if (cur) os << " ";
    }
    return os;
}

Resolver::Resolver (const LoopSP& loop, uint32_t expiration_time, size_t limit)
    : loop(loop.get()), expiration_time(expiration_time), limit(limit), destroyed()
{
    _ECTOR();
    ares_options options;
    int optmask = 0;

    options.sock_state_cb      = ares_sockstate_cb;
    options.sock_state_cb_data = this;
    optmask |= ARES_OPT_SOCK_STATE_CB;

    options.flags = ARES_FLAG_NOALIASES;
    optmask |= ARES_OPT_FLAGS;

    if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) throw Error("resolver couldn't init c-ares");

    dns_roll_timer = new Timer(loop);
    dns_roll_timer->weak(true);
    dns_roll_timer->timer_event.add([=](Timer*) {
        _EDEBUG("dns roll timer");
        ares_process_fd(channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    });

    loop->destroy_event.add(on_loop_destroy = [this](const LoopSP&){
        destroy();
    });
}

void Resolver::on_delete () {
    _EDTOR();
    destroy();
}

void Resolver::destroy () {
    if (destroyed) return;
    _EDEBUGTHIS();
    destroyed = true;

    ares_destroy(channel);
    assert(!connections.size());

    for (auto& request : requests) request->cancel();
    requests.clear();

    loop->destroy_event.remove(on_loop_destroy);

    dns_roll_timer = nullptr;
    loop = nullptr;
}

ResolveRequestSP Resolver::resolve (const Builder& p) {
    check_alive();
    _EDEBUGTHIS("use_cache: %d", p._use_cache);
    ResolveRequestSP req = new ResolveRequest(p._callback, this);
    requests.push_back(req);

    auto service = p._service;
    char port_cstr[5];
    if (p._port) {
        auto res = std::to_chars(port_cstr, port_cstr + sizeof(port_cstr), p._port);
        assert(!res.ec);
        service = string_view(port_cstr, res.ptr - port_cstr);
    }

    if (p._use_cache) {
        auto ai = find(p._node, service, p._hints);
        if (ai) {
            auto reqptr = req.get();
            loop->delay([=]{
                call_now(reqptr, ai, nullptr);
                this->remove_request(reqptr);
            }, this);
            return req;
        }
    }

    if (p._use_cache) req->key = new ResolverCacheKey(string(p._node), string(service), p._hints);

    PEXS_NULL_TERMINATE(p._node, node_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    auto reqp = req.get();
    if (p._timeout) req->timer = Timer::once(p._timeout, [reqp](const TimerSP&) {
        _EDEBUG("timed out req:%p", reqp);
        reqp->resolver->call_now(reqp, nullptr, CodeError(std::errc::timed_out));
    }, loop);

    _EDEBUGTHIS("req:%p [%s:%s] loop:%p use_cache:%d", req.get(), node_cstr, service_cstr, loop, p._use_cache);
    ares_addrinfo h = { p._hints.flags, p._hints.family, p._hints.socktype, p._hints.protocol, 0, nullptr, nullptr, nullptr };
    ares_getaddrinfo(channel, node_cstr, service.length() ? service_cstr : nullptr, &h, ares_resolve_cb, req.get());
    req->ares_async = true;
    return req;
}

void Resolver::reset () {
    if (destroyed) return;
    _EDEBUGTHIS();

    dns_roll_timer->stop();

    ares_cancel(channel);
    connections.clear();

    for (auto& request : requests) request->cancel();
    requests.clear();
}

void Resolver::ares_sockstate_cb (void* data, sock_t sock, int read, int write) {
    auto resolver = static_cast<Resolver*>(data);
    _EDEBUG("resolver:%p sock:%d read:%d write:%d", resolver, sock, read, write);

    if (read || write) {
        auto& conn = resolver->connections[sock];

        if (!conn) {
            if (!resolver->dns_roll_timer->active()) resolver->dns_roll_timer->start(DNS_ROLL_TIMEOUT);
            conn = new Poll(Poll::Socket{sock}, resolver->loop);
        }

        conn->start((read ? Poll::READABLE : 0) | (write ? Poll::WRITABLE : 0), [=](Poll*, int, const CodeError*) {
            ares_process_fd(resolver->channel, sock, sock);
        });
    } else {
        // c-ares notifies us that the socket is closed
        resolver->connections.erase(sock);
        if (resolver->connections.empty()) resolver->dns_roll_timer->stop();
    }
}

void Resolver::ares_resolve_cb (void* arg, int status, int, ares_addrinfo* ai) {
    auto req      = static_cast<ResolveRequest*>(arg);
    auto resolver = req->resolver;
    _EDEBUG("req:%p status:%s async:%d ai:%p", req, ares_strerror(status), req->ares_async, ai);

    if (req->done) {
        _EDEBUG("ignoring cancelled request");
        resolver->remove_request(req);
        return;
    }

    CodeError err;
    AddrInfo  addr;
    switch (status) {
        case ARES_SUCCESS:
            addr = AddrInfo(ai);
            #if EVENT_LIB_DEBUG >= 1
            _EDEBUG("addr: %.*s", (int)addr.to_string().length(), addr.to_string().data());
            #endif
            break;
        case ARES_ECANCELLED:
        case ARES_EDESTRUCTION:
            err = CodeError(std::errc::operation_canceled);
            break;
        case ARES_ENOTIMP:
            err = CodeError(std::errc::address_family_not_supported);
            break;
        case ARES_ENOMEM:
            err = CodeError(std::errc::not_enough_memory);
            break;
        case ARES_ENOTFOUND:
        default:
            err = CodeError(errc::resolve_error);
    }

    if (req->ares_async) {
        resolver->call_now(req, addr, err);
        resolver->remove_request(req);
    } else {
        resolver->loop->delay([=]{
            resolver->call_now(req, addr, err);
            resolver->remove_request(req);
        }, resolver);
    }
}

void Resolver::on_resolve (const ResolveRequestSP& req, const AddrInfo& addr, const CodeError* err) {
    req->event(this, req, addr, err);
}

void Resolver::call_now (const ResolveRequestSP& req, const AddrInfo& addr, const CodeError* err) {
    _EDEBUGTHIS("request:%p err:%d use_cache:%d", req.get(), err ? err->code().value() : 0, (bool)req->key);
    if (req->done) return;
    req->done  = true;
    req->timer = nullptr;

    if (!err && req->key) {
        if (cache.size() >= limit) {
            _EDEBUG("cleaning cache %p %ld", this, cache.size());
            cache.clear();
        }
        cache.emplace(*req->key, CachedAddress{addr});
    }

    on_resolve(req, addr, err);
}

void Resolver::remove_request (ResolveRequest* req) {
    requests.erase(req);
    req->resolver = nullptr;
    if (!requests.size()) {
        for (auto& row : connections) row.second->stop();
    }
}

AddrInfo Resolver::find (std::string_view node, std::string_view service, const AddrInfoHints& hints) {
    _EDEBUGTHIS("looking in cache [%.*s:%.*s] cache_size: %zd", (int)node.length(), node.data(), (int)service.length(), service.data(), cache.size());
    auto it = cache.find({string(node), string(service), hints});
    if (it != cache.end()) {
        _EDEBUGTHIS("found in cache [%.*s]", (int)node.length(), node.data());

        time_t now = time(0);
        if (!it->second.expired(now, expiration_time)) return it->second.address;

        _EDEBUGTHIS("expired [%.*s]", (int)node.length(), node.data());
        cache.erase(it);
    }
    return {};
}

void Resolver::clear_cache () {
    cache.clear();
}

ResolveRequest::ResolveRequest (resolve_fn callback, Resolver* resolver) : resolver(resolver), done(), ares_async() {
    _ECTOR();
    event.add(callback);
}

ResolveRequest::~ResolveRequest () { _EDTOR(); }

void ResolveRequest::cancel () {
    if (!resolver) return; // request has already been completed
    resolver->call_now(this, nullptr, CodeError(std::errc::operation_canceled));
}

}}
