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

namespace panda { namespace unievent {

// addrinfo extract, there are fields needed for hinting only
struct AddrInfoHints {
    AddrInfoHints() : family(INT_MIN), socktype(INT_MIN), protocol(INT_MIN), flags(INT_MIN) {}

    AddrInfoHints(int family = AF_UNSPEC, int socktype = SOCK_STREAM, int proto = 0, int flags = AI_PASSIVE) : 
        family(family), socktype(socktype), protocol(proto), flags(flags) {}

    AddrInfoHints(const AddrInfoHints& oth) = default;

    bool operator==(const AddrInfoHints& oth) const {
        return family == oth.family && socktype == oth.socktype && protocol == oth.protocol && flags == oth.flags;
    }

    template <typename T> T to() const {
        return T { flags, family, socktype, protocol, 0, nullptr, nullptr, nullptr };
    }

    int family;
    int socktype;
    int protocol;
    int flags;
};

struct AddrInfo : Refcnt {
    explicit AddrInfo(ares_addrinfo* ai) : head(new DataSource(ai)), cur(ai) {}

    int              flags()     const { return cur->ai_flags; }
    int              family()    const { return cur->ai_family; }
    int              socktype()  const { return cur->ai_socktype; }
    int              protocol()  const { return cur->ai_protocol; }
    net::SockAddr    addr()      const { return cur->ai_addr; }
    std::string_view canonname() const { return cur->ai_canonname; }
    AddrInfo         next()      const { return AddrInfo(src, cur->ai_next); }

    explicit operator bool() const { return cur; }

    //void detach() { head = nullptr; }

    std::string to_string();

private:
    struct DataSource : Refcnt {
        ares_addrinfo* ai;
        ~AresWrapper() { ares_freeaddrinfo(ai); }
    };

    iptr<DataSource> src;
    ares_addrinfo*   cur;

    AddrInfo(const iptr<DataSource>& src, ares_addrinfo* cur) : src(src), cur(cur) {}
};

std::ostream& operator<<(std::ostream& os, const AddrInfo& ai);

struct CachedAddress {
    CachedAddress(const AddrInfo& ai, std::time_t update_time = std::time(0)) : address(ai), update_time(update_time) {}

    bool expired(time_t now, time_t expiration_time) const { return update_time + expiration_time < now; }

    AddrInfo    address;
    std::time_t update_time;
};

struct ResolverCacheHash;
struct ResolverCacheKey : Refcnt {
    friend ResolverCacheHash;

    ResolverCacheKey(const string& node, const string& service, const AddrInfoHints& hints) : node(node), service(service), hints(hints) {}

    bool operator==(const ResolverCacheKey& other) const {
        return node == other.node && service == other.service && hints == other.hints;
    }

private:
    string        node;
    string        service;
    AddrInfoHints hints;
};

struct ResolverCacheHash {
    template <class T> inline void hash_combine(std::size_t& seed, const T& v) const {
        seed ^= std::hash<T>(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    std::size_t operator()(const ResolverCacheKey& p) const {
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

struct AresTask : virtual Refcnt {
    ~AresTask() {
        _EDTOR();
        if (poll) {
            poll->stop();
        }
    }

    AresTask(Loop* loop) : loop(loop) {
        _ECTOR();
    }

    void start(sock_t sock, int events, Poll::poll_fn callback) {
        if (!poll)
            poll = new Poll(-1, sock, loop);

        poll->start(events, callback);
    }

    LoopSP loop;
    PollSP poll;
};

struct Resolver : virtual Refcnt {
    static constexpr uint64_t DEFAULT_RESOLVE_TIMEOUT       = 1000;  // [ms]
    static constexpr time_t   DEFAULT_CACHE_EXPIRATION_TIME = 20*60; // [s]
    static constexpr size_t   DEFAULT_CACHE_LIMIT           = 10000; // [records]

    // keep in sync with xsi constants
    enum { 
        UE_AI_CANONNAME   = ARES_AI_CANONNAME,
        UE_AI_NUMERICSERV = ARES_AI_NUMERICSERV
    };

    Resolver(const LoopSP& loop = Loop::default_loop(), time_t expiration_time = DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = DEFAULT_CACHE_LIMIT);
    Resolver(Resolver& other) = delete;
    Resolver& operator=(Resolver& other) = delete;
    ~Resolver();

    // resolve if not in cache and save in cache afterwards
    // will trigger expunge if the cache is too big
    virtual ResolveRequestSP resolve(
            std::string_view node,
            std::string_view service,
            const AddrInfoHints& hints,
            ResolveFunction callback,
            bool use_cache = true);

    virtual void stop();

    // search in cache, will remove the record if expired
    std::tuple<ResolverCacheType::const_iterator, bool> find(std::string_view node, std::string_view service, const AddrInfoHints& hints);

    size_t cache_size() const { return cache.size(); }

    void clear_cache() { cache.clear(); }

    bool expunge_cache() {
        if (cache.size() >= limit_) {
            _EDEBUG("cleaning cache %p %ld", this, cache_.size());
            cache.clear();
            return true;
        }
        return false;
    }

    virtual void call_on_resolve(ResolveRequest* resolve_request, const AddrInfoSP& addr, CodeError err);
protected:
    virtual void on_resolve(SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err = nullptr);
    
private:
    using AresTasks = std::map<sock_t, AresTaskSP>;

    LoopSP            loop;
    ares_channel      channel;
    TimerSP           timer;
    ResolverCacheType cache;
    time_t            expiration_time;
    size_t            limit;
    AresTasks         tasks;

    static void ares_resolve_cb(void *arg, int status, int timeouts, ares_addrinfo* ai);
    static void ares_sockstate_cb(void* data, sock_t sock, int read, int write);
};

struct ResolveRequest : virtual Refcnt, AllocatedObject<ResolveRequest, true> {
    ~ResolveRequest(); 
    ResolveRequest(ResolveFunction callback, SimpleResolver* resolver);

    virtual void cancel();

    CallbackDispatcher<ResolveFunctionPlain> event;
    SimpleResolver* resolver;
    ResolverCacheKeySP key;
    bool async;
    bool done;
};

}} // namespace panda::unievent
