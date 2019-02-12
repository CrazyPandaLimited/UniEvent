#pragma once
#include "Loop.h"
#include "Poll.h"
#include "Timer.h"
#include "Debug.h"

#include <map>
#include <ctime>
#include <ares.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <panda/string_view.h>
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent {

// addrinfo extract, there are fields needed for hinting only
struct AddrInfoHints {
    static constexpr const int PASSIVE     = ARES_AI_PASSIVE;
    static constexpr const int CANONNAME   = ARES_AI_CANONNAME;
    static constexpr const int NUMERICSERV = ARES_AI_NUMERICSERV;

    AddrInfoHints (int family = AF_UNSPEC, int socktype = 0, int proto = 0, int flags = 0) :
        family(family), socktype(socktype), protocol(proto), flags(flags) {}

    AddrInfoHints (const AddrInfoHints& oth) = default;

    bool operator== (const AddrInfoHints& oth) const {
        return family == oth.family && socktype == oth.socktype && protocol == oth.protocol && flags == oth.flags;
    }

    int family;
    int socktype;
    int protocol;
    int flags;
};

struct AddrInfo : Refcnt {
    AddrInfo()                  : cur(nullptr) {}
    AddrInfo(ares_addrinfo* ai) : src(new DataSource(ai)), cur(ai) {}

    int              flags     () const { return cur->ai_flags; }
    int              family    () const { return cur->ai_family; }
    int              socktype  () const { return cur->ai_socktype; }
    int              protocol  () const { return cur->ai_protocol; }
    net::SockAddr    addr      () const { return cur->ai_addr; }
    std::string_view canonname () const { return cur->ai_canonname; }
    AddrInfo         next      () const { return AddrInfo(src, cur->ai_next); }
    AddrInfo         first     () const { return AddrInfo(src, src->ai); }

    explicit operator bool () const { return cur; }

    bool operator== (const AddrInfo& oth) const;
    bool operator!= (const AddrInfo& oth) const { return !operator==(oth); }

    bool is (const AddrInfo& oth) const { return cur == oth.cur; }

    //void detach() { head = nullptr; }

    std::string to_string ();

private:
    struct DataSource : Refcnt {
        ares_addrinfo* ai;
        DataSource (ares_addrinfo* ai) : ai(ai) {}
        ~DataSource () { ares_freeaddrinfo(ai); }
    };

    iptr<DataSource> src;
    ares_addrinfo*   cur;

    AddrInfo (const iptr<DataSource>& src, ares_addrinfo* cur) : src(src), cur(cur) {}
};

std::ostream& operator<< (std::ostream& os, const AddrInfo& ai);

struct CachedAddress {
    CachedAddress (const AddrInfo& ai, std::time_t update_time = std::time(0)) : address(ai), update_time(update_time) {}

    bool expired (time_t now, time_t expiration_time) const { return update_time + expiration_time < now; }

    AddrInfo    address;
    std::time_t update_time;
};

struct ResolverCacheHash;
struct ResolverCacheKey : Refcnt {
    friend ResolverCacheHash;

    ResolverCacheKey (const string& node, const string& service, const AddrInfoHints& hints) : node(node), service(service), hints(hints) {}

    bool operator== (const ResolverCacheKey& other) const {
        return node == other.node && service == other.service && hints == other.hints;
    }

private:
    string        node;
    string        service;
    AddrInfoHints hints;
};

struct ResolverCacheHash {
    template <class T> inline void hash_combine (std::size_t& seed, const T& v) const {
        seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    std::size_t operator() (const ResolverCacheKey& p) const {
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

using ResolverCacheKeySP = iptr<ResolverCacheKey>;
using ResolverCacheType  = std::unordered_map<ResolverCacheKey, CachedAddress, ResolverCacheHash>;

struct ResolveRequest;
using ResolveRequestSP = iptr<ResolveRequest>;

struct ResolveRequest : panda::lib::IntrusiveChainNode<ResolveRequestSP>, Refcnt, panda::lib::AllocatedObject<ResolveRequest, true> {
    using resolve_fptr = void(const ResolverSP&, const ResolveRequestSP&, const AddrInfo& address, const CodeError* err);
    using resolve_fn   = function<resolve_fptr>;

    ~ResolveRequest ();

    virtual void cancel ();

    CallbackDispatcher<resolve_fptr> event;

private:
    friend Resolver;

    ResolveRequest (resolve_fn callback, Resolver* resolver);

    Resolver*          resolver;
    ResolverCacheKeySP key;
    TimerSP            timer;
    bool               async;
    bool               done;
};


struct Resolver : virtual Refcnt {
    static constexpr uint64_t DEFAULT_RESOLVE_TIMEOUT       = 5000;  // [ms]
    static constexpr uint32_t DEFAULT_CACHE_EXPIRATION_TIME = 20*60; // [s]
    static constexpr size_t   DEFAULT_CACHE_LIMIT           = 10000; // [records]

    using resolve_fn = ResolveRequest::resolve_fn;

    struct Builder {
        std::string_view _node;
        std::string_view _service;
        uint16_t         _port;
        AddrInfoHints    _hints;
        resolve_fn       _callback;
        bool             _use_cache;
        uint64_t         _timeout;

        Builder (Resolver* r) : _port(0), _use_cache(true), _timeout(DEFAULT_RESOLVE_TIMEOUT), _resolver(r), _called(false) {}

        Builder& node       (std::string_view val)     { _node      = val; return *this; }
        Builder& service    (std::string_view val)     { _service   = val; return *this; }
        Builder& port       (uint16_t val)             { _port      = val; return *this; }
        Builder& hints      (const AddrInfoHints& val) { _hints     = val; return *this; }
        Builder& on_resolve (const resolve_fn& val)    { _callback  = val; return *this; }
        Builder& use_cache  (bool val)                 { _use_cache = val; return *this; }
        Builder& timeout    (uint64_t val)             { _timeout   = val; return *this; }

        ResolveRequestSP run () { return _resolver->resolve(*this); }

    private:
        Resolver* _resolver;
        bool      _called;
    };

    Resolver (const LoopSP& loop = Loop::default_loop(), uint32_t expiration_time = DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = DEFAULT_CACHE_LIMIT);
    ~Resolver ();

    Resolver (Resolver& other) = delete;
    Resolver& operator= (Resolver& other) = delete;

    Builder resolve () { return Builder(this); }

    ResolveRequestSP resolve (std::string_view node, resolve_fn callback, uint64_t timeout = DEFAULT_RESOLVE_TIMEOUT) {
        return resolve().node(node).on_resolve(callback).timeout(timeout).run();
    }

    virtual void reset ();

    uint32_t cache_expiration_time () const { return expiration_time; }
    size_t   cache_limit           () const { return limit; }
    size_t   cache_size            () const { return cache.size(); }

    void cache_expiration_time (uint32_t val) { expiration_time = val; }

    void cache_limit (size_t val) {
        limit = val;
        clear_cache();
    }

    void clear_cache ();

    virtual void call_now (const ResolveRequestSP&, const AddrInfo&, const CodeError*);

    void release () const {
        if (refcnt() <= 1) const_cast<Resolver*>(this)->destroy();
        Refcnt::release();
    }

protected:
    virtual ResolveRequestSP resolve (const Builder&);

    virtual void on_resolve (const ResolveRequestSP&, const AddrInfo&, const CodeError* = nullptr);
    
private:
    struct Connection {
        PollSP  poll;
        TimerSP timer;
    };

    using Connections = std::map<sock_t, PollSP>;
    using Requests    = panda::lib::IntrusiveChain<ResolveRequestSP>;

    weak<LoopSP>      loop;
    ares_channel      channel;
    TimerSP           dns_roll_timer;
    ResolverCacheType cache;
    time_t            expiration_time;
    size_t            limit;
    Connections       connections;
    Requests          requests;

    LoopSP get_loop () const {
        auto ret = loop.lock();
        if (!ret) throw Error("Loop has been destroyed and this resolver can not be used anymore");
        return ret;
    }

    // search in cache, will remove the record if expired
    AddrInfo find (std::string_view node, std::string_view service, const AddrInfoHints& hints);

    void remove_request (ResolveRequest* req);

    void destroy ();

    static void ares_resolve_cb (void* arg, int status, int timeouts, ares_addrinfo* ai);
    static void ares_sockstate_cb (void* data, sock_t sock, int read, int write);
};

inline void refcnt_dec (const Resolver* o) { o->release(); }

}}
