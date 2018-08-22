#pragma once

#include <panda/unievent/CachedResolver.h>
#include <panda/unievent/ResolveFunction.h>
#include <panda/unievent/Socks.h>
#include <panda/unievent/Stream.h>
#include <panda/unievent/Timer.h>
#include <panda/unievent/socks/SocksProxy.h>
#include <panda/string.h>
#include <panda/string_view.h>

namespace panda { namespace unievent {

constexpr bool use_cached_resolver_by_default = true; 

inline string to_string(const sockaddr* sa) {
    switch(sa->sa_family) {
        case AF_INET: {
            size_t maxlen = INET_ADDRSTRLEN;
            char addr[maxlen];
            sockaddr_in* src = (sockaddr_in*)sa;
            if(uv_inet_ntop(AF_INET, &src->sin_addr, addr, maxlen)) {
                return string("");
            } else {
                return string(addr, strlen(addr)) + ':' + panda::to_string(ntohs(src->sin_port));
            }
        }
        case AF_INET6: {
            size_t maxlen = INET6_ADDRSTRLEN;
            sockaddr_in6* src = (sockaddr_in6*)sa;
            char addr[maxlen];
            if(uv_inet_ntop(AF_INET6, &src->sin6_addr, addr, maxlen)) {
                return string("");
            } else {
                return string(addr, strlen(addr)) + ':' + panda::to_string(ntohs(src->sin6_port));
            }
        }
        default:
            return string("unknown address family");
    }
}

inline string to_string(const addrinfo* ai) {
    return to_string(ai->ai_addr);
}

struct addrinfo_deleter {
    void operator()(addrinfo* ptr) {
        if (ptr != nullptr)
            freeaddrinfo(ptr);
    }
};

using addrinfo_keeper = std::unique_ptr<addrinfo, addrinfo_deleter>;

class Resolver;
class CachedResolver;
class ConnectRequest;
class ResolveRequest;
class TCP : public virtual Stream, public AllocatedObject<TCP> {
    /* SOCKS
    friend socks::SocksProxy;
    */

public:
    ~TCP();

    TCP (Loop* loop = Loop::default_loop());
    
    TCP (Loop* loop, unsigned int flags);

    virtual void open(sock_t socket);
    virtual void bind(const sockaddr* sa, unsigned int flags = 0);
    virtual void bind(std::string_view host, std::string_view service, const addrinfo* hints = &defhints);

    virtual bool resolve(const string& host, const string& service, const addrinfo* hints, ResolveFunction callback, bool use_cached_resolver);

    virtual void connect(const sockaddr* sa, ConnectRequest* connect_request = nullptr);

    virtual void connect(
        const string& host, const string& service,
        const addrinfo* hints = nullptr,
        ConnectRequest* connect_request = nullptr,
        bool use_cached_resolver = use_cached_resolver_by_default);

    void connect(const sockaddr* sa, connect_fn callback) { 
        return connect(sa, new ConnectRequest(callback)); 
    }

    virtual void reconnect(const sockaddr* sa, ConnectRequest* connect_request = nullptr);

    virtual void reconnect(
        const string& host, const string& service,
        const addrinfo* hints = nullptr,
        ConnectRequest* connect_request = nullptr,
        bool use_cached_resolver = use_cached_resolver_by_default);
    /* SOCKS
    void use_socks(std::string_view host, uint16_t port = 1080, std::string_view login = "", std::string_view passw = "", bool socks_resolve = true);

    bool use_socks() const { return socks_proxy; }
    */

    struct ConnectBuilder;
    ConnectBuilder connect() {return ConnectBuilder(this);}

    struct ReconnectBuilder;
    ReconnectBuilder reconnect() {return ReconnectBuilder(this);}

    void tcp_nodelay (bool enable) {
        int err = uv_tcp_nodelay(&uvh, enable);
        if (err) throw TCPError(err);
    }

    void tcp_keepalive (bool enable, unsigned int delay) {
        int err = uv_tcp_keepalive(&uvh, enable, delay);
        if (err) throw TCPError(err);
    }

    void tcp_simultaneous_accepts (bool enable) {
        int err = uv_tcp_simultaneous_accepts(&uvh, enable);
        if (err) throw TCPError(err);
    }

    void getsockname (sockaddr* name, int* namelen) {
        int err = uv_tcp_getsockname(&uvh, name, namelen);
        if (err) throw TCPError(err);
    }

    void getpeername (sockaddr* name, int* namelen) {
        int err = uv_tcp_getpeername(&uvh, name, namelen);
        if (err) throw TCPError(err);
    }

#ifdef _WIN32
    void setsockopt (int level, int optname, const void* optval, int optlen) {
    	if (::setsockopt(fileno(), level, optname, (const char*)optval, optlen)) throw TCPError(WSAGetLastError());
    }
#else
    void setsockopt (int level, int optname, const void* optval, socklen_t optlen) {
    	if (::setsockopt(fileno(), level, optname, optval, optlen)) throw TCPError(-errno);
    }
#endif

    string dump() const {
        string sockname_str, peername_str;
        sockaddr sockname, peername;
        int namelen;
        int res;

        namelen = sizeof sockname;
        res = uv_tcp_getsockname(&uvh, &sockname, &namelen);
        if(res) {
            sockname_str = string("error");
        } else {
            sockname_str = to_string(&sockname);
        }

        namelen = sizeof peername;
        res = uv_tcp_getpeername(&uvh, &peername, &namelen);
        if(res) {
            peername_str = string("error");
        } else {
            peername_str = to_string(&peername);
        }
         
        return string("local:") + sockname_str + string(" peer:") + peername_str + string(" connected:") + (connected() ? "yes" : "no"); 
    }

    struct ConnectBuilder {
        friend TCP;

        ConnectBuilder& to(const string& host, const string& service, const addrinfo* hints = nullptr) {
            host_ = host;
            service_ = service;
            info_ = hints;
            return *this;
        }

        ConnectBuilder& to(const sockaddr* sa) {sa_ = sa; return *this;}
        ConnectBuilder& timeout(uint64_t timeout) {timeout_ = timeout; return *this;}
        ConnectBuilder& request(ConnectRequestSP request) {connect_request_ = request; return *this;}
        ConnectBuilder& cached_resolver(bool enabled) {cached_resolver_ = enabled; return *this;}
        /* SOCKS
        ConnectBuilder& socks(std::string_view host, uint16_t port = 1080, std::string_view login = "", std::string_view passw = "", bool socks_resolve = true) { socks_ = new Socks(string(host), port, string(login), string(passw), socks_resolve); return *this; }
        ConnectBuilder& socks(SocksSP socks) {socks_ = socks; return *this;}
        */

        ~ConnectBuilder() noexcept(false);

    private:
        ConnectBuilder(TCP* tcp) : tcp_(tcp) {}
        ConnectBuilder(const ConnectBuilder&) = default;
        ConnectBuilder(ConnectBuilder&&) = default;

        TCP* tcp_ = nullptr;
        const sockaddr* sa_ = nullptr;
        string host_ = "";
        string service_ = "";
        const addrinfo* info_ = nullptr;
        uint64_t timeout_ = 0;
        ConnectRequestSP connect_request_;
        bool cached_resolver_ = use_cached_resolver_by_default;
        /* SOCKS
        SocksSP socks_;
        */
    };

    struct ReconnectBuilder : ConnectBuilder {
        friend TCP;

        ~ReconnectBuilder() noexcept(false);
    private:
        ReconnectBuilder(TCP* tcp) : ConnectBuilder(tcp) {}
        ReconnectBuilder(const ReconnectBuilder&) = default;
        ReconnectBuilder(ReconnectBuilder&&) = default;
    };

protected:
    void on_handle_reinit () override;
    void close_reinit(bool keep_asyncq = false) override;
    virtual void _close() override;
    bool resolve_with_cached_resolver(const string& host, const string& service, const addrinfo* hints, ResolveFunction callback);
    bool resolve_with_regular_resolver(const string& host, const string& service, const addrinfo* hints, ResolveFunction callback);
    /* SOCKS
    void use_socks(SocksSP socks);
    */

private:
    uv_tcp_t uvh;
    static addrinfo defhints;
    ResolverSP resolver;
    ResolveRequestSP resolve_request;
    /* SOCKS
    socks::SocksProxySP socks_proxy;
    */
    TimerSP connect_timer;
};

using TCPSP = iptr<TCP>;

in_port_t find_free_port();

}}
