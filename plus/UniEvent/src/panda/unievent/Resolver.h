#pragma once

#include <map>
#include <ctime>
#include <cstdlib>
#include <unordered_map>

#include <ares.h>

#include <panda/string_view.h>

#include "Debug.h"
#include "Loop.h"
#include "Poll.h"
#include "Handle.h"
#include "Request.h"
#include "ResolveFunction.h"
#include "Timer.h"
#include "global.h"

namespace panda { namespace unievent {

constexpr uint64_t DEFAULT_RESOLVE_TIMEOUT = 1000; // [ms]

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

    explicit AddrInfo(ares_addrinfo* addr) : head(addr) {}

    AddrInfo(AddrInfo&& other) {
        head       = other.head;
        other.head = 0;
    }

    AddrInfo& operator=(AddrInfo&& other) {
        head       = other.head;
        other.head = 0;
        return *this;
    }

    AddrInfo(AddrInfo& other) = delete;
    AddrInfo& operator=(AddrInfo& other) = delete;

    void detach() { head = nullptr; }

    ares_addrinfo* head;
};

struct AddressRotator : AddrInfo {
    ~AddressRotator() {}
    
    AddressRotator(AddrInfoSP other) : AddrInfo(std::move(*other)) {
        init();
    }

    AddressRotator(ares_addrinfo* addr) : AddrInfo(addr) {
        init();
    }
    
    AddressRotator(AddressRotator& other) = delete; 
    AddressRotator& operator=(AddressRotator& other) = delete;

    // rotate everything in cache (round robin, ignore RFC6724)
    ares_addrinfo* rotate() {
        if (current->ai_next) {
            current = current->ai_next;
        } else {
            current = head;
        }

        return current;
    }

    ares_addrinfo* current;

private:
    void init() {
        length_ = 0;
        for (auto res = head; res; res = res->ai_next) {
            ++length_;
        }

        // get random element and set it as initial
        if (length_) {
            size_t pos        = 0;
            size_t random_pos = rand() % length_;
            _EDEBUG("init to %ld %ld", length_, random_pos);
            for (auto res = head; res; res = res->ai_next) {
                if (pos++ >= random_pos) {
                    current = res;
                    return;
                }
            }
        } else {
            current = head;
        }
    }

    size_t length_;
};

struct CachedAddress : AddressRotator {
    CachedAddress(CachedAddress& other) = delete; 
    CachedAddress& operator=(CachedAddress& other) = delete;

    CachedAddress(AddrInfoSP address, std::time_t update_time = std::time(0)) : AddressRotator(address), update_time(update_time) {}

    bool expired(time_t now, time_t expiration_time) const { return update_time + expiration_time < now; }

    std::time_t update_time;
};

namespace cached_resolver {

constexpr time_t DEFAULT_CACHE_EXPIRATION_TIME = 300;
constexpr size_t DEFAULT_CACHE_LIMIT           = 10000;

struct Hash;
struct Key : virtual Refcnt {
    friend Hash;

public:
    Key(const string& node, const string& service, const AddrInfoHintsSP& hints) : node_(node), service_(service), hints_(hints) {}
    bool operator==(const Key& other) const { return node_ == other.node_ && service_ == other.service_ && hints_ == other.hints_; }

private:
    string          node_;
    string          service_;
    AddrInfoHintsSP hints_;
};

struct Hash {
    template <class T> inline void hash_combine(std::size_t& seed, const T& v) const {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    std::size_t operator()(const Key& p) const {
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

inline string to_string(const Key& key) { return string::from_number(cached_resolver::Hash{}(key), 16); }

typedef iptr<CachedAddress> Value;

} // namespace cached_resolver

struct AresTask : virtual Refcnt {
    ~AresTask() {
        if (poll) {
            poll->stop();
        }
    }

    AresTask(Loop* loop) : loop(loop) {}

    void start(sock_t sock, int events, Poll::poll_fn callback) {
        if (!poll)
            poll = new Poll(-1, sock, loop);

        poll->start(events, callback);
    }

    LoopSP loop;
    PollSP poll;
};

struct Resolver : virtual Refcnt {

    // keep in sync with xsi constants
    enum { 
        UE_AI_CANONNAME   = ARES_AI_CANONNAME,
        UE_AI_NUMERICSERV = ARES_AI_NUMERICSERV
    };

    ~Resolver();
    Resolver(Loop* loop);
    Resolver(Resolver& other) = delete; 
    Resolver& operator=(Resolver& other) = delete;

    virtual void resolve(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints, ResolveFunction callback = nullptr);

protected:
    virtual void on_resolve(ResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err);
    
private:
    static void ares_resolve_cb(void *arg, int status, int timeouts, ares_addrinfo* ai);
    static void ares_sockstate_cb(void* data, sock_t sock, int read, int write);
    
public:
    Loop*        loop;
    TimerSP      timer;
    ares_channel channel;

    using AresTasks = std::map<sock_t, AresTaskSP>;
    AresTasks tasks;
};

struct CachedResolver : Resolver {
    using CacheType = std::unordered_map<cached_resolver::Key, cached_resolver::Value, cached_resolver::Hash>;

    ~CachedResolver();

    CachedResolver(Loop* loop, time_t expiration_time = cached_resolver::DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = cached_resolver::DEFAULT_CACHE_LIMIT);
    
    CachedResolver(CachedResolver& other) = delete; 
    CachedResolver& operator=(CachedResolver& other) = delete;

    // search in cache, will remove the record if expired
    std::tuple<CacheType::const_iterator, bool> find(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints);

    // resolve if not in cache and save in cache afterwards
    // will trigger expunge if the cache is too big
    void resolve(std::string_view node, std::string_view service, const AddrInfoHintsSP& hints, ResolveFunction callback = nullptr) override;

    size_t cache_size() const { return cache_.size(); }

    void clear() { cache_.clear(); }

protected:
    void on_resolve(ResolverSP resolver, ResolveRequestSP resolve_request, AddrInfoSP address, const CodeError* err) override;

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
    CacheType          cache_;
    time_t             expiration_time_;
    size_t             limit_;
};

struct ResolveRequest : virtual Refcnt, AllocatedObject<ResolveRequest, true> {
    ~ResolveRequest(); 
    ResolveRequest(ResolveFunction callback);
    
    CallbackDispatcher<ResolveFunctionPlain> event;
    Resolver*                                resolver;
    iptr<cached_resolver::Key>               key;
    bool                                     async;
};

inline Resolver* get_thread_local_simple_resolver(Loop* loop) {
    thread_local ResolverSP resolver(new Resolver(loop));
    return resolver.get();
}

inline CachedResolver* get_thread_local_cached_resolver(Loop* loop) {
    thread_local CachedResolverSP resolver(new CachedResolver(loop));
    return resolver.get();
}

inline void clear_resolver_cache(Loop* loop) { get_thread_local_cached_resolver(loop)->clear(); }

}} // namespace panda::event
