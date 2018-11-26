#pragma once

#include <map>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <unordered_map>

#include <ares.h>

#include <panda/string_view.h>

#include "Fwd.h"
#include "Debug.h"
#include "Loop.h"
#include "Poll.h"
#include "Handle.h"
#include "Request.h"
#include "Timer.h"
#include "global.h"
#include "Request.h"

namespace panda { namespace unievent {

// addrinfo extract, there are fields needed for hinting only
struct AddrInfoHints : virtual Refcnt {
    AddrInfoHints(int family = AF_UNSPEC, int socktype = SOCK_STREAM, int proto = 0, int flags = AI_PASSIVE) : 
        ai_family(family), ai_socktype(socktype), ai_protocol(proto), ai_flags(flags) {}

    bool operator==(const AddrInfoHints& other) const {
        return ai_family == other.ai_family && ai_socktype == other.ai_socktype && ai_protocol == other.ai_protocol && ai_flags == other.ai_flags;
    }

    AddrInfoHintsSP clone() const {
        return new AddrInfoHints(ai_family, ai_socktype, ai_protocol, ai_flags);
    }

    template <typename T> T to() const {
        return T {
            ai_flags,
            ai_family,
            ai_socktype,
            ai_protocol,
            0,
            nullptr,
            nullptr,
            nullptr
        };
    }

    int ai_family;
    int ai_socktype;
    int ai_protocol;
    int ai_flags;
};

struct AddrInfo : virtual Refcnt {
    ~AddrInfo() {
        if (head) {
            ares_freeaddrinfo(head);
        }
    }

    explicit AddrInfo(ares_addrinfo* addr) : head(addr) {
    }

    AddrInfo(AddrInfo&& other) {
        head       = other.head;
        other.head = 0;
    }

    AddrInfo& operator=(AddrInfo&& other) {
        head       = other.head;
        other.head = 0;
        return *this;
    }

    AddrInfo(const AddrInfo& other) = delete;
    AddrInfo& operator=(const AddrInfo& other) = delete;

    void detach() { head = nullptr; }

    std::string to_string();

    ares_addrinfo* head;
};

std::ostream& operator<<(std::ostream& os, const AddrInfo& ai);

struct CachedAddress {
    CachedAddress(AddrInfoSP address, std::time_t update_time = std::time(0)) : address(address), update_time(update_time) {
    }

    bool expired(time_t now, time_t expiration_time) const { return update_time + expiration_time < now; }

    AddrInfoSP  address;
    std::time_t update_time;
};

struct ResolverCacheHash;
struct ResolverCacheKey : virtual Refcnt {
    friend ResolverCacheHash;

    ResolverCacheKey(const string& node, const string& service, const AddrInfoHintsSP& hints) : node_(node), service_(service), hints_(hints) {}
    bool operator==(const ResolverCacheKey& other) const 
    {
        if(hints_ == other.hints_) { 
            // same hints or nullptr hints
            return node_ == other.node_ && service_ == other.service_; 
        }
        else if(hints_ && other.hints_) { 
            // some comparable hints
            return node_ == other.node_ && service_ == other.service_ && *hints_ == *other.hints_; 
        } else {
            // different hints
            return false;
        }
    }

private:
    string          node_;
    string          service_;
    AddrInfoHintsSP hints_;
};

struct ResolverCacheHash {
    template <class T> inline void hash_combine(std::size_t& seed, const T& v) const {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    std::size_t operator()(const ResolverCacheKey& p) const {
        std::size_t seed = 0;
        hash_combine(seed, p.node_);
        hash_combine(seed, p.service_);
        hash_combine(seed, p.hints_->ai_flags);
        hash_combine(seed, p.hints_->ai_family);
        hash_combine(seed, p.hints_->ai_socktype);
        hash_combine(seed, p.hints_->ai_protocol);
        return seed;
    }
};

using ResolverCacheKeySP   = iptr<ResolverCacheKey>;
using ResolverCacheType    = std::unordered_map<ResolverCacheKey, CachedAddress, ResolverCacheHash>;

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

struct SimpleResolver : virtual Refcnt {
    // channel inactivity timeout 
    static constexpr uint64_t DEFAULT_RESOLVE_TIMEOUT = 1000; // [ms]

    // keep in sync with xsi constants
    enum { 
        UE_AI_CANONNAME   = ARES_AI_CANONNAME,
        UE_AI_NUMERICSERV = ARES_AI_NUMERICSERV
    };

    ~SimpleResolver();
    SimpleResolver(Loop* loop);
    SimpleResolver(SimpleResolver& other) = delete; 
    SimpleResolver& operator=(SimpleResolver& other) = delete;

    virtual ResolveRequestSP resolve(
            std::string_view node,
            std::string_view service,
            const AddrInfoHintsSP& hints,
            ResolveFunction callback,
            bool use_cache = false);

    virtual void stop();

protected:
    virtual void on_resolve(SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err = nullptr);
    
private:
    static void ares_resolve_cb(void *arg, int status, int timeouts, ares_addrinfo* ai);
    static void ares_sockstate_cb(void* data, sock_t sock, int read, int write);

    TimerSP timer;

public:
    Loop*        loop;
    ares_channel channel;

    using AresTasks = std::map<sock_t, AresTaskSP>;
    AresTasks tasks;
};

struct Resolver : SimpleResolver {
    static constexpr time_t DEFAULT_CACHE_EXPIRATION_TIME = 300;
    static constexpr size_t DEFAULT_CACHE_LIMIT           = 10000;

    ~Resolver();

    Resolver(Loop* loop, time_t expiration_time = DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = DEFAULT_CACHE_LIMIT);
    
    Resolver(Resolver& other) = delete; 
    Resolver& operator=(Resolver& other) = delete;

    // search in cache, will remove the record if expired
    std::tuple<ResolverCacheType::const_iterator, bool> find(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints);

    // resolve if not in cache and save in cache afterwards
    // will trigger expunge if the cache is too big
    ResolveRequestSP resolve(
            std::string_view node,
            std::string_view service,
            const AddrInfoHintsSP& hints,
            ResolveFunction callback,
            bool use_cache = true) override;

    size_t cache_size() const { return cache_.size(); }

    void clear() { cache_.clear(); }

protected:
    void on_resolve(SimpleResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err = nullptr) override;

private:
    bool expunge_cache() {
        if (cache_.size() >= limit_) {
            _EDEBUG("cleaning cache %p %ld", this, cache_.size());
            cache_.clear();
            return true;
        }
        return false;
    }

private:
    ResolverCacheType cache_;
    time_t            expiration_time_;
    size_t            limit_;
};

struct ResolveRequest : virtual Refcnt, AllocatedObject<ResolveRequest, true> {
    ~ResolveRequest(); 
    ResolveRequest(ResolveFunction callback, SimpleResolver* resolver);

    CallbackDispatcher<ResolveFunctionPlain> event;
    SimpleResolver* resolver;
    ResolverCacheKeySP key;
    bool async;
    bool canceled;
};

}} // namespace panda::unievent
