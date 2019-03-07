#pragma once
#include "Loop.h"
#include "Poll.h"
#include "Timer.h"
#include "Debug.h"
#include "Request.h"
#include "AddrInfo.h"

#include <ctime>
#include <vector>
#include <ares.h>
#include <cstdlib>
#include <unordered_map>
#include <panda/string_view.h>

namespace panda { namespace unievent {

struct Resolver : Refcnt, private backend::ITimerListener {
    static constexpr uint64_t DEFAULT_RESOLVE_TIMEOUT       = 5000;  // [ms]
    static constexpr uint32_t DEFAULT_CACHE_EXPIRATION_TIME = 20*60; // [s]
    static constexpr size_t   DEFAULT_CACHE_LIMIT           = 10000; // [records]

    struct Request;
    using RequestSP = iptr<Request>;

    using resolve_fptr = void(const RequestSP&, const AddrInfo& address, const CodeError* err);
    using resolve_fn   = function<resolve_fptr>;

    static ResolverSP create_loop_resolver (const LoopSP& loop, uint32_t exptime = DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = DEFAULT_CACHE_LIMIT);

    Resolver (const LoopSP& loop = Loop::default_loop(), uint32_t exptime = DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = DEFAULT_CACHE_LIMIT);

    Resolver (Resolver& other) = delete;
    Resolver& operator= (Resolver& other) = delete;

    LoopSP loop () const { return _loop; }

    RequestSP resolve ();
    RequestSP resolve (string node, resolve_fn callback, uint64_t timeout = DEFAULT_RESOLVE_TIMEOUT);

    virtual void resolve (const RequestSP&);

    virtual void reset ();

    AddrInfo find (const string& node, const string& service, const AddrInfoHints& hints);

    uint32_t cache_expiration_time () const { return expiration_time; }
    size_t   cache_limit           () const { return limit; }
    size_t   cache_size            () const { return cache.size(); }

    void cache_expiration_time (uint32_t val) { expiration_time = val; }

    void cache_limit (size_t val) {
        limit = val;
        clear_cache();
    }

    void clear_cache ();

protected:
    virtual void on_resolve (const RequestSP&, const AddrInfo&, const CodeError* = nullptr);

    ~Resolver ();

private:
    struct CachedAddress {
        CachedAddress (const AddrInfo& ai, std::time_t update_time = std::time(0)) : address(ai), update_time(update_time) {}

        bool expired (time_t now, time_t expiration_time) const { return update_time + expiration_time < now; }

        AddrInfo    address;
        std::time_t update_time;
    };

    struct CacheKey : Refcnt {
        CacheKey (const string& node, const string& service, const AddrInfoHints& hints) : node(node), service(service), hints(hints) {}

        bool operator== (const CacheKey& other) const {
            return node == other.node && service == other.service && hints == other.hints;
        }

        string        node;
        string        service;
        AddrInfoHints hints;
    };

    struct CacheHash {
        template <class T> inline void hash_combine (std::size_t& seed, const T& v) const {
            seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        std::size_t operator() (const CacheKey& p) const {
            std::size_t seed = 0;
            hash_combine(seed, p.node);
            hash_combine(seed, p.service);
            hash_combine(seed, p.hints.flags);
            hash_combine(seed, p.hints.family);
            hash_combine(seed, p.hints.socktype);
            hash_combine(seed, p.hints.protocol);
            return seed;
        }
    };

    using CacheKeySP = iptr<CacheKey>;
    using Cache = std::unordered_map<CacheKey, CachedAddress, CacheHash>;
    using BTimer = backend::BackendTimer;
    using BPoll  = backend::BackendPoll;

    struct Worker : private backend::ITimerListener, private backend::IPollListener {
        Worker  (Resolver*);
        ~Worker ();

        void on_sockstate (sock_t sock, int read, int write);

        void resolve    (const RequestSP&);
        void on_resolve (int status, int timeouts, ares_addrinfo* ai);

        void finish_resolve (const AddrInfo&, const CodeError* err);
        void cancel ();

        void on_timer () override;
        void on_poll  (int, const CodeError*) override;

        Resolver*    resolver;
        ares_channel channel;
        sock_t       sock;
        BPoll*       poll;
        BTimer*      timer;
        RequestSP    request;
        bool         ares_async;
    };

    using Requests = panda::lib::IntrusiveChain<RequestSP>;
    using Workers  = std::vector<std::unique_ptr<Worker>>;

    Loop*    _loop;
    LoopSP   _loop_hold;
    BTimer*  dns_roll_timer;
    Workers  workers;
    Requests queue;
    Requests cache_delayed;
    Cache    cache;
    time_t   expiration_time;
    size_t   limit;

    Resolver (uint32_t expiration_time, size_t limit, Loop* loop);

    void add_worker ();

    void finish_resolve (const RequestSP&, const AddrInfo&, const CodeError*);

    void on_timer () override;

    friend Request; friend Worker;
};

struct Resolver::Request : Refcnt, panda::lib::IntrusiveChainNode<Resolver::RequestSP>, panda::lib::AllocatedObject<Resolver::Request> {
    CallbackDispatcher<resolve_fptr> event;

    Request (const ResolverSP& r = {});

    const ResolverSP& resolver () const { return _resolver; }

    RequestSP node       (string val)               { _node      = val; return this; }
    RequestSP service    (string val)               { _service   = val; return this; }
    RequestSP port       (uint16_t val)             { _port      = val; return this; }
    RequestSP hints      (const AddrInfoHints& val) { _hints     = val; return this; }
    RequestSP on_resolve (const resolve_fn& val)    { event.add(val);   return this; }
    RequestSP use_cache  (bool val)                 { _use_cache = val; return this; }
    RequestSP timeout    (uint64_t val)             { _timeout   = val; return this; }

    RequestSP run () {
        RequestSP self = this;
        _resolver->resolve(self);
        return self;
    }

    void cancel ();

protected:
    ~Request ();

private:
    friend Resolver;

    LoopSP        loop;      // keep loop (for loop resolvers where resolver doesn't have strong ref to loop)
    ResolverSP    _resolver; // keep resolver
    string        _node;
    string        _service;
    uint16_t      _port;
    AddrInfoHints _hints;
    resolve_fn    _callback;
    bool          _use_cache;
    uint64_t      _timeout;
    CacheKeySP    key;
    Worker*       worker;
    uint64_t      delayed;
    bool          running;
    bool          queued;
};

inline Resolver::RequestSP Resolver::resolve () { return new Request(this); }

inline Resolver::RequestSP Resolver::resolve (string node, resolve_fn callback, uint64_t timeout) {
    return resolve()->node(node)->on_resolve(callback)->timeout(timeout)->run();
}

}}
