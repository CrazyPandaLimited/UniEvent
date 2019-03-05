#include "Resolver.h"
#include "Timer.h"
#include "Prepare.h"
#include <algorithm>
#include <functional>

namespace panda { namespace unievent {

static constexpr const int    DNS_ROLL_TIMEOUT = 1000; // [ms]
static constexpr const size_t MAX_WORKERS      = 5;

Resolver::Worker::Worker (Resolver* r) : resolver(r), sock(), poll(), timer(), ares_async() {
    _ECTOR();
    ares_options options;
    int optmask = 0;

    options.sock_state_cb_data = this;
    options.sock_state_cb      = [](void* arg, sock_t sock, int read, int write) {
        static_cast<Worker*>(arg)->on_sockstate(sock, read, write);
    };
    optmask |= ARES_OPT_SOCK_STATE_CB;

    options.flags = ARES_FLAG_NOALIASES;
    optmask |= ARES_OPT_FLAGS;

    if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) throw Error("resolver couldn't init c-ares");
}

Resolver::Worker::~Worker () {
    _EDTOR();
    if (poll) poll->destroy();
    if (timer) timer->destroy();
    ares_destroy(channel);
}

void Resolver::Worker::on_sockstate (sock_t sock, int read, int write) {
    _EDEBUG("resolver:%p sock:%d read:%d write:%d", resolver, sock, read, write);

    if (!read && !write) { // c-ares notifies us that the socket is closed
        assert(this->sock == sock);
        poll->destroy();
        poll = nullptr;
        //if (resolver->connections.empty()) resolver->dns_roll_timer->stop();
        return;
    }

    if (!poll) {
        this->sock = sock;
        poll = resolver->_loop->impl()->new_poll_sock(this, sock);
    }
    else assert(this->sock == sock);

    poll->start((read ? Poll::READABLE : 0) | (write ? Poll::WRITABLE : 0));
}

void Resolver::Worker::on_poll (int, const CodeError*) {
    ares_process_fd(channel, sock, sock);
}

void Resolver::Worker::resolve (const RequestSP& req) {
    request = req;
    request->worker = this;

    if (req->_timeout) {
        if (!timer) timer = resolver->_loop->impl()->new_timer(this);
        timer->start(0, req->_timeout);
    }

    PEXS_NULL_TERMINATE(req->_node, node_cstr);
    PEXS_NULL_TERMINATE(req->_service, service_cstr);
    _EDEBUGTHIS("req:%p [%s:%s] use_cache:%d", req.get(), node_cstr, service_cstr, req->_use_cache);

    ares_addrinfo h = { req->_hints.flags, req->_hints.family, req->_hints.socktype, req->_hints.protocol, 0, nullptr, nullptr, nullptr };
    ares_async = false;
    ares_getaddrinfo(
        channel,
        node_cstr,
        req->_service.length() ? service_cstr : nullptr,
        &h,
        [](void* arg, int status, int timeouts, ares_addrinfo* ai){
            static_cast<Worker*>(arg)->on_resolve(status, timeouts, ai);
        },
        this
    );
    ares_async = true;
}

void Resolver::Worker::on_timer () {
    _EDEBUG("timed out req:%p", request.get());
    finish_resolve(nullptr, CodeError(std::errc::timed_out));
}

void Resolver::Worker::on_resolve (int status, int, ares_addrinfo* ai) {
    _EDEBUG("req:%p status:%s async:%d ai:%p", request.get(), ares_strerror(status), ares_async, ai);
    if (!request) return;

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

    if (ares_async) {
        finish_resolve(addr, err);
    } else {
        request->delayed = resolver->loop()->delayer.add([=]{
            request->delayed = 0;
            finish_resolve(addr, err);
        });
    }
}

void Resolver::Worker::cancel () {
    if (!request) return;
    request->worker = nullptr;
    request = nullptr;
    if (timer) timer->stop();
    ares_cancel(channel);
}

void Resolver::Worker::finish_resolve (const AddrInfo& addr, const CodeError* err) {
    if (timer) timer->stop();
    request->worker = nullptr;
    auto req = std::move(request);
    resolver->finish_resolve(req, addr, err);
}


ResolverSP Resolver::create_loop_resolver (const LoopSP& loop, uint32_t exptime, size_t limit) {
    return new Resolver(exptime, limit, loop.get());
}

Resolver::Resolver (const LoopSP& loop, uint32_t exptime, size_t limit) : Resolver(exptime, limit, loop.get()) {
    _loop_hold = loop;
}

Resolver::Resolver (uint32_t expiration_time, size_t limit, Loop* loop) : _loop(loop), expiration_time(expiration_time), limit(limit) {
    _ECTOR();
    add_worker();
    dns_roll_timer = _loop->impl()->new_timer(this);
    dns_roll_timer->set_weak();
}

Resolver::~Resolver () {
    _EDTOR();
    for (auto& w : workers) assert(!w || !w->request);
    assert(!queue.size());
    dns_roll_timer->destroy();
}

void Resolver::on_timer () {
    _EDEBUG("dns roll timer");
    for (auto& w : workers) if (w && w->request) ares_process_fd(w->channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
}

void Resolver::add_worker () {
    assert(workers.size() < MAX_WORKERS);
    auto worker = new Worker(this);
    workers.emplace_back(worker);
}

void Resolver::resolve (const RequestSP& req) {
    if (req->_port) req->_service = string::from_number(req->_port);
    _EDEBUGTHIS("use_cache: %d", req->_use_cache);
    req->resolver = this;
    req->running  = true;
    req->loop     = _loop; // keep loop (for loop resolvers)

    if (req->_use_cache) {
        auto ai = find(req->_node, req->_service, req->_hints);
        if (ai) {
            req->delayed = loop()->delayer.add([=]{
                req->delayed = 0;
                finish_resolve(req, ai, nullptr);
            });
            return;
        }
        req->key = new CacheKey(req->_node, req->_service, req->_hints);
    }

    if (queue.size()) {
        queue.push_back(req);
        return;
    }

    for (auto& w : workers) {
        if (w->request) continue;
        if (!dns_roll_timer->active()) dns_roll_timer->start(DNS_ROLL_TIMEOUT, DNS_ROLL_TIMEOUT);
        w->resolve(req);
        return;
    }

    if (workers.size() < MAX_WORKERS) {
        add_worker();
        workers.back()->resolve(req);
    } else {
        queue.push_back(req);
    }
}

void Resolver::finish_resolve (const RequestSP& req, const AddrInfo& addr, const CodeError* err) {
    if (!req->running) return;
    _EDEBUGTHIS("request:%p err:%d use_cache:%d", req.get(), err ? err->code().value() : 0, (bool)req->key);
    req->running = false;

    if (req->delayed) loop()->delayer.cancel(req->delayed);

    auto worker = req->worker;

    if (worker) {
        worker->cancel();
    } else {
        queue.erase(req);
    }

    if (!err && req->key) {
        if (cache.size() >= limit) {
            _EDEBUG("cleaning cache %p %ld", this, cache.size());
            cache.clear();
        }
        cache.emplace(*req->key, CachedAddress{addr});
    }

    on_resolve(req, addr, err);

    req->loop = nullptr; // release loop (for loop resolvers)

    if (worker && !worker->request) { // worker might have been used again in callback
        if (queue.size()) {
            worker->resolve(queue.front());
            queue.pop_front();
        } else { // worker became free, check if any requests left
            bool busy = false;
            for (auto& w : workers) if (w->request) {
                busy = true;
                break;
            }
            if (!busy) dns_roll_timer->stop();
        }
    }
}

void Resolver::on_resolve (const RequestSP& req, const AddrInfo& addr, const CodeError* err) {
    req->event(req, addr, err);
}

void Resolver::reset () {
    _EDEBUGTHIS();

    dns_roll_timer->stop();

    if (queue.size()) { // cancel only till last as cancel() might add new requests
        auto last = queue.back();
        while (queue.front() != last) queue.front()->cancel();
        last->cancel();
    }

    // some workers may start new resolve on cancel() because new request might be added on cancel()
    for (auto& w : workers) if (w->request) w->request->cancel();
}

AddrInfo Resolver::find (const string& node, const string& service, const AddrInfoHints& hints) {
    _EDEBUGTHIS("looking in cache [%.*s:%.*s] cache_size: %zd", (int)node.length(), node.data(), (int)service.length(), service.data(), cache.size());
    auto it = cache.find({node, service, hints});
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

Resolver::Request::~Request () { _EDTOR(); }

void Resolver::Request::cancel () {
    if (resolver) resolver->finish_resolve(this, nullptr, CodeError(std::errc::operation_canceled));
}

}}
