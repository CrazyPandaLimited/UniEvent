#pragma once
#include <ctime>
#include <cstdlib>
#include <unordered_map>
#include <panda/string_view.h>

#include "Loop.h"
#include "Debug.h"
#include "global.h"
#include "Request.h"
#include "ResolveFunction.h"

namespace panda { namespace unievent {

struct AbstractResolver;
using AbstractResolverSP = iptr<AbstractResolver>;

struct Resolver;
using ResolverSP = iptr<Resolver>;

struct CachedResolver;
using CachedResolverSP = iptr<CachedResolver>;

struct ResolveRequest;
using ResolveRequestSP = iptr<ResolveRequest>;

struct AddressRotator;
using AddressRotatorSP = iptr<AddressRotator>;

struct CachedAddress;
using CachedAddressSP = iptr<CachedAddress>;

struct BasicAddress : virtual Refcnt {
    explicit BasicAddress (addrinfo* addr) : head(addr) {}

    ~BasicAddress () {
        if (head) uv_freeaddrinfo(head);
    }

    BasicAddress (BasicAddress&& other) {
        head       = other.head;
        other.head = 0;
    }

    BasicAddress& operator= (BasicAddress&& other) {
        head       = other.head;
        other.head = 0;
        return *this;
    }

    BasicAddress (const BasicAddress& other) = delete;
    BasicAddress& operator= (const BasicAddress& other) = delete;
   
    void detach () { head = nullptr; }

    addrinfo* head;
};
using BasicAddressSP = iptr<BasicAddress>;

struct AddressRotator : BasicAddress {
    AddressRotator (BasicAddressSP other) : BasicAddress(std::move(*other)) {
        init();
    }

    AddressRotator (addrinfo* addr) : BasicAddress(addr) {
        init();
    }
    
    AddressRotator (const AddressRotator& other) = delete;
    AddressRotator& operator= (const AddressRotator& other) = delete;

    // rotate everything in cache
    addrinfo* next () {
        current = current->ai_next ? current->ai_next : head;
        return current;
    }

    addrinfo* current;

private:
    void init () {
        length_ = 0;
        for (auto res = head; res; res = res->ai_next) ++length_;

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
    CachedAddress (const CachedAddress& other) = delete;
    CachedAddress& operator= (const CachedAddress& other) = delete;

    CachedAddress (BasicAddressSP address, std::time_t update_time = std::time(0)) : AddressRotator(address), update_time(update_time) {}

    bool expired (time_t now, time_t expiration_time) const { return update_time + expiration_time < now; }

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
    Key (const string& node, uint16_t port, const addrinfo* hints) : node(node), port(port) {
        if (hints) {
            this->hints.ai_flags    = hints->ai_flags;
            this->hints.ai_family   = hints->ai_family;
            this->hints.ai_socktype = hints->ai_socktype;
            this->hints.ai_protocol = hints->ai_protocol;
        }
    }

    bool operator== (const Key& other) const { return node == other.node && port == other.port && hints == other.hints; }

private:
    friend Hash;

    string   node;
    uint16_t port;
    Hints    hints;
};

struct Hash {
    template <class T> inline void hash_combine(std::size_t& seed, const T& v) const {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    std::size_t operator()(const Key& p) const {
        std::size_t seed = 0;
        hash_combine(seed, p.node);
        hash_combine(seed, p.port);
        hash_combine(seed, p.hints.ai_flags);
        hash_combine(seed, p.hints.ai_family);
        hash_combine(seed, p.hints.ai_socktype);
        hash_combine(seed, p.hints.ai_protocol);
        return seed;
    }
};

inline string to_string(const Key& key) { return string::from_number(cached_resolver::Hash{}(key), 16); }

typedef iptr<CachedAddress> Value;

} // namespace cached_resolver

struct AbstractResolver : virtual Refcnt {
    virtual ResolveRequestSP resolve (Loop* loop, std::string_view node, uint16_t port, const addrinfo* hints, ResolveFunction callback) = 0;
    virtual void on_resolve (AbstractResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) = 0;
};

struct Resolver : AbstractResolver {
    Resolver () {}

    ResolveRequestSP resolve (Loop*            loop,
                              std::string_view node,
                              uint16_t         port = 0,
                              const addrinfo*  hints    = nullptr,
                              ResolveFunction  callback = nullptr) override;
    
    Resolver (const Resolver& other) = delete;
    Resolver& operator= (const Resolver& other) = delete;

protected:
    void on_resolve (AbstractResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) override;
};

struct CachedResolver : Resolver {
    using CacheType = std::unordered_map<cached_resolver::Key, cached_resolver::Value, cached_resolver::Hash>;

    CachedResolver (time_t expiration_time = cached_resolver::DEFAULT_CACHE_EXPIRATION_TIME, size_t limit = cached_resolver::DEFAULT_CACHE_LIMIT);
    
    CachedResolver (const CachedResolver& other) = delete;
    CachedResolver& operator= (const CachedResolver& other) = delete;

    // search in cache, will remove the record if expired
    std::tuple<CacheType::const_iterator, bool>
    find (std::string_view node, uint16_t port = 0, const addrinfo* hints = nullptr);

    // resolve if not in cache and save in cache afterwards
    // will trigger expunge if the cache is too big
    ResolveRequestSP resolve (Loop*            loop,
                              std::string_view node,
                              uint16_t         port = 0,
                              const addrinfo*  hints    = nullptr,
                              ResolveFunction  callback = nullptr) override;

    size_t cache_size () const { return cache_.size(); }

    void clear () { cache_.clear(); }

protected:
    void on_resolve (AbstractResolverSP resolver, ResolveRequestSP resolve_request, BasicAddressSP address, const CodeError* err) override;

private:
    bool expunge_cache () {
        if (cache_.size() >= limit_) {
            _EDEBUG("cleaning cache %p %ld", this, cache_.size());
            cache_.clear();
            return true;
        }
        return false;
    }

    CacheType cache_;
    time_t    expiration_time_;
    size_t    limit_;
};

struct Request;
struct ConnectRequest;
struct ResolveRequest : CancelableRequest, AllocatedObject<ResolveRequest, true> {
    CallbackDispatcher<ResolveFunctionPlain> event;

    ResolveRequest (ResolveFunction callback);

    void cancel() override;

    AbstractResolver*          resolver;
    iptr<cached_resolver::Key> key;
};

inline Resolver* get_global_basic_resolver () {
    static ResolverSP resolver(new Resolver());
    return resolver.get();
}

inline CachedResolver* get_thread_local_cached_resolver () {
    thread_local CachedResolverSP resolver(new CachedResolver());
    return resolver.get();
}

inline void clear_resolver_cache () { get_thread_local_cached_resolver()->clear(); }

}}
