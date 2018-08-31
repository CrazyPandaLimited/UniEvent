#pragma once

#include <unordered_map>
#include <cstdlib>

#include <panda/string_view.h>
#include <panda/unievent/global.h>
#include <panda/unievent/Loop.h>
#include <panda/unievent/Request.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/ResolveFunction.h>

namespace panda { namespace unievent { namespace cached_resolver {

struct AddressBase : public virtual Refcnt {
    AddressBase(addrinfo*& addr) : head(addr) {
    }

    addrinfo* head;
};

struct AddressRotator : public AddressBase {
    ~AddressRotator(){
        _ETRACETHIS("dtor");
    }

    AddressRotator(addrinfo* addr) : AddressBase(addr) {
        _ETRACETHIS("ctor");
        length_ = 0;
        for(auto res = head; res; res = res->ai_next) {
            ++length_;
        }

        // get random element and set it as initial
        if(length_) {
            size_t pos = 0;
            size_t random_pos = rand() % length_;
            _EDEBUG("init to %zd %zd", length_, random_pos);
            for(auto res = addr; res; res = res->ai_next) {
                if(pos++ >= random_pos) {
                    current = res;
                    return;
                }
            }
        } else {
            current = addr;
        }
    }

    // rotate everything in cache
    addrinfo* next() {
        if(current->ai_next) {
            current = current->ai_next;
        } else {
            current = head;
        }

        return current; 
    }

public:
    addrinfo* current;

private:
    size_t length_;
};

struct Address : public AddressRotator {
    ~Address() {
        _ETRACETHIS("dtor");
        if(head) {
            uv_freeaddrinfo(head); 
        }
    }

    Address(addrinfo* addr, time_t update_time) : AddressRotator(addr), update_time(update_time) {
        _ETRACETHIS("ctor");
    }

    bool expired(time_t now, time_t expiration_time) const {
        return update_time + expiration_time < now;
    }

    time_t update_time;
};

// addrinfo extract, there are fields needed for hinting only
struct Hints {
    bool operator==(const Hints &other) const { 
        return ai_flags == other.ai_flags 
            && ai_family == other.ai_family
            && ai_socktype == other.ai_socktype
            && ai_protocol == other.ai_protocol; 
    }

    int ai_flags = AI_PASSIVE;
    int ai_family = PF_UNSPEC;
    int ai_socktype = SOCK_STREAM;
    int ai_protocol = 0;
};

struct Hash;
class Key : public virtual Refcnt {
    friend Hash;
public:
    Key(const string& node, const string& service, const addrinfo* hints) : node_(node), service_(service) {
        if(hints) {
            hints_.ai_flags = hints->ai_flags;
            hints_.ai_family = hints->ai_family;
            hints_.ai_socktype = hints->ai_socktype;
            hints_.ai_protocol = hints->ai_protocol;
        }
    }

    bool operator==(const Key &other) const { 
        return node_ == other.node_ && service_ == other.service_ && hints_ == other.hints_; 
    }

private:
    string node_;
    string service_;
    Hints hints_; 
};

struct Hash {
    template <class T>                                                                                                                               
    inline void hash_combine(std::size_t& seed, const T& v) const {                                                                                  
        std::hash<T> hasher;                                                                                                                         
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);                                                                                      
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

inline
string to_string(const Key& key) {
    return string::from_number(cached_resolver::Hash{}(key), 16);
}

using Value = iptr<Address>;

} // namespace cached_resolver

struct CachedResolver;

class ResolveRequest : public CancelableRequest, public AllocatedObject<ResolveRequest, true> {
public:
    // XXX this API with Resolver as first argument here for backward compatibility, remove this when the time is right
    using resolve_fptr = void(CachedResolver* req, addrinfo* res, const ResolveError& err, bool from_cache);
    using resolve_fn   = function<resolve_fptr>;

    CallbackDispatcher<resolve_fptr> event_compat;

    CallbackDispatcher<ResolveFunctionPlain> event;

    ~ResolveRequest() {
        _ETRACETHIS("dtor");
    }

    ResolveRequest(resolve_fn callback);

    ResolveRequest(ResolveFunction callback);

    friend uv_getaddrinfo_t* _pex_(ResolveRequest*);

public:
    CachedResolver* resolver;
    ConnectRequest* connect_request;
    iptr<cached_resolver::Key> key;

private:
    uv_getaddrinfo_t uvr;
};
using ResolveRequestSP = iptr<ResolveRequest>;

struct CachedResolver : virtual Refcnt {
    using resolve_fptr = void(CachedResolver* resolver, addrinfo* res, const ResolveError& err, bool from_cache);
    using resolve_fn = function<resolve_fptr>;

    typedef std::unordered_map<cached_resolver::Key, cached_resolver::Value, cached_resolver::Hash> CacheType;

    ~CachedResolver();

    CachedResolver(time_t expiration_time = 300, size_t limit = 10000);
  
    // try search cache
    // will remove the record if expired 
    std::tuple<CacheType::const_iterator, bool> find(std::string_view node, 
            std::string_view service = std::string_view(), const addrinfo* hints = nullptr) {
        iptr<cached_resolver::Key> key(new cached_resolver::Key(string(node), string(service), hints));
       
        _EDEBUG("looking in cache [%.*s] [%.*s] %zd", 
                (int)node.length(), node.data(), 
                (int)service.length(), service.data(), 
                cache_.size());

        auto address_pos = cache_.find(*key);
        if(address_pos != end(cache_)) {
            _EDEBUG("node in cache [%.*s]", (int)node.length(), node.data());

            time_t now = time(0);
            if(address_pos->second->expired(now, expiration_time_)) {
                cache_.erase(address_pos);
            } else {
                return std::make_tuple<CacheType::const_iterator, bool>(address_pos, true);
            } 
        }

        return std::make_tuple<CacheType::const_iterator, bool>(address_pos, false);
    }
    
    // resolve if not in cache and save in cache afterwards
    // will trigger expunge if cache is too big 
    ResolveRequestSP resolve_async (Loop*, std::string_view node, std::string_view service, const addrinfo* hints = NULL, ResolveFunction cb = nullptr);

    // find and resolve_async combined
    // intended for an external cached resolver usage
    ResolveRequestSP resolve (Loop* loop, std::string_view node, std::string_view service = {}, const addrinfo* hints = NULL, ResolveFunction cb = nullptr) {
        CacheType::const_iterator address_pos;
        bool found;
        std::tie(address_pos, found) = find(node, service, hints);
        if (found && cb) {
            cb(address_pos->second->head, ResolveError(0), true);
            return nullptr;
        }

        return resolve_async(loop, node, service, hints, cb);
    }

    ResolveRequestSP resolve_async_compat (Loop* loop, std::string_view node, std::string_view service, const addrinfo* hints = NULL, resolve_fn cb = nullptr);

    ResolveRequestSP resolve_compat (Loop* loop, std::string_view node, std::string_view service = {}, const addrinfo* hints = NULL, resolve_fn cb = nullptr) {
        CacheType::const_iterator address_pos;
        bool found;
        std::tie(address_pos, found) = find(node, service, hints);
        if (found && cb) {
            cb(this, address_pos->second->head, ResolveError(0), true);
            return nullptr;
        }

        return resolve_async_compat(loop, node, service, hints, cb);
    }
    
    size_t cache_size() const { return cache_.size(); }
    
    void clear() { cache_.clear(); }

private:
    bool expunge_cache () {
        if (cache_.size() >= limit_) {
            _EDEBUG("cleaning cache %p %zd", this, cache_.size());
            cache_.clear();
            return true;
        }
        return false;
    }

    static void uvx_on_resolve (uv_getaddrinfo_t* req, int status, addrinfo* res);

private:
    CacheType cache_;
    time_t expiration_time_;
    size_t limit_;
};
using CachedResolverSP = iptr<CachedResolver>;

inline uv_getaddrinfo_t* _pex_ (ResolveRequest* req) { return &req->uvr; }

inline CachedResolver* get_thread_local_cached_resolver () {
    thread_local CachedResolverSP resolver(new CachedResolver());
    return resolver.get();
}

inline void clear_resolver_cache () {
    get_thread_local_cached_resolver()->clear();
}

}}

