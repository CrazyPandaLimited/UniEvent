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

struct Resolver;
using ResolverSP = iptr<Resolver>;

struct CachedResolver;
using CachedResolverSP = iptr<CachedResolver>;

struct ResolveRequest;
using ResolveRequestSP = iptr<ResolveRequest>;

struct BasicAddress;
using BasicAddressSP = iptr<BasicAddress>;

struct AddressRotator;
using AddressRotatorSP = iptr<AddressRotator>;

struct CachedAddress;
using CachedAddressSP = iptr<CachedAddress>;

struct BasicAddress : virtual Refcnt {
    ~BasicAddress() {
        if (head) {
            uv_freeaddrinfo(head);
        }
    }

    explicit BasicAddress(addrinfo* addr) : head(addr) {}

    BasicAddress(BasicAddress&& other) {
        head       = other.head;
        other.head = 0;
    }

    BasicAddress& operator=(BasicAddress&& other) {
        head       = other.head;
        other.head = 0;
        return *this;
    }

    BasicAddress(BasicAddress& other) = delete; 
    BasicAddress& operator=(BasicAddress& other) = delete;
   
    void detach() { head = nullptr; }

    addrinfo* head;
};

struct AddressRotator : BasicAddress {
    ~AddressRotator() {}
    
    AddressRotator(BasicAddressSP other) : BasicAddress(std::move(*other)) {
        init();
    }

    AddressRotator(addrinfo* addr) : BasicAddress(addr) {
        init();
    }
    
    AddressRotator(AddressRotator& other) = delete; 
    AddressRotator& operator=(AddressRotator& other) = delete;

    // rotate everything in cache (round robin, ignore RFC6724)
    addrinfo* next() {
        if (current->ai_next) {
            current = current->ai_next;
        } else {
            current = head;
        }

        return current;
    }

    addrinfo* current;

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

    CachedAddress(BasicAddressSP address, std::time_t update_time = std::time(0)) : AddressRotator(address), update_time(update_time) {}

    bool expired(time_t now, time_t expiration_time) const { return update_time + expiration_time < now; }

    std::time_t update_time;
};

namespace cached_resolver {

constexpr time_t DEFAULT_CACHE_EXPIRATION_TIME = 300;
constexpr size_t DEFAULT_CACHE_LIMIT           = 10000;

// addrinfo extract, there are fields needed for hinting only
struct Hints {
    bool operator==(const Hints& other) const {
        return ai_flags == other.ai_flags && ai_family == other.ai_family && ai_socktype == other.ai_socktype && ai_protocol == other.ai_protocol;
    }

    int ai_flags    = AI_PASSIVE;
    int ai_family   = PF_UNSPEC;
    int ai_socktype = SOCK_STREAM;
    int ai_protocol = 0;
};

struct Hash;
struct Key : virtual Refcnt {
    friend Hash;

public:
    Key(const string& node, const string& service, const addrinfo* hints) : node_(node), service_(service) {
        if (hints) {
            hints_.ai_flags    = hints->ai_flags;
            hints_.ai_family   = hints->ai_family;
            hints_.ai_socktype = hints->ai_socktype;
            hints_.ai_protocol = hints->ai_protocol;
        }
    }

    bool operator==(const Key& other) const { return node_ == other.node_ && service_ == other.service_ && hints_ == other.hints_; }

private:
    string node_;
    string service_;
    Hints  hints_;
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
        hash_combine(seed, p.hints_.ai_flags);
        hash_combine(seed, p.hints_.ai_family);
        hash_combine(seed, p.hints_.ai_socktype);
        hash_combine(seed, p.hints_.ai_protocol);
        return seed;
    }
};

inline string to_string(const Key& key) { return string::from_number(cached_resolver::Hash{}(key), 16); }

typedef iptr<CachedAddress> Value;

} // namespace cached_resolver

struct Resolver : virtual Refcnt {
    ~Resolver();
    Resolver(Loop* loop);
    Resolver(Resolver& other) = delete; 
    Resolver& operator=(Resolver& other) = delete;

    virtual ResolveRequestSP resolve(Loop*            loop,
                             std::string_view node,
                             std::string_view service  = std::string_view(),
                             const addrinfo*  hints    = nullptr,
                             ResolveFunction  callback = nullptr);

protected:
    virtual void on_resolve(ResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err);
    
private:
    static void ares_resolve_cb(void *arg, int status,int timeouts, struct hostent *hostent);
    static void ares_sockstate_cb(void* data, sock_t sock, int read, int write);
    
public:
    Loop*        loop;
    TimerSP      timer;
    ares_channel channel;

    using Requests = std::map<sock_t, ResolveRequestSP>;
    Requests requests;
};

struct CachedResolver : Resolver {
    using CacheType = std::unordered_map<cached_resolver::Key, cached_resolver::Value, cached_resolver::Hash>;

    ~CachedResolver();

    CachedResolver(Loop* loop, time_t expiration_time = cached_resolver::DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = cached_resolver::DEFAULT_CACHE_LIMIT);
    
    CachedResolver(CachedResolver& other) = delete; 
    CachedResolver& operator=(CachedResolver& other) = delete;

    // search in cache, will remove the record if expired
    std::tuple<CacheType::const_iterator, bool>
    find(std::string_view node, std::string_view service = std::string_view(), const addrinfo* hints = nullptr);

    // resolve if not in cache and save in cache afterwards
    // will trigger expunge if the cache is too big
    ResolveRequestSP resolve(Loop*            loop,
                             std::string_view node,
                             std::string_view service  = std::string_view(),
                             const addrinfo*  hints    = nullptr,
                             ResolveFunction  callback = nullptr) override;

    size_t cache_size() const { return cache_.size(); }

    void clear() { cache_.clear(); }

protected:
    void on_resolve(ResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) override;

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
    
    void cancel() {
    }

    bool active() const {
        return active_;
    }
    
    void activate(sock_t sock); 
    
    void close(); 

    sock_t                                   socket;
    PollSP                                   poll;
    CallbackDispatcher<ResolveFunctionPlain> event;
    Resolver*                                resolver;
    iptr<cached_resolver::Key>               key;
    bool                                     async;

private:
    bool active_;
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
