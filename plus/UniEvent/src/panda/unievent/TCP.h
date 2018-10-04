#pragma once

#include <ostream>

#include <panda/unievent/Resolver.h>
#include <panda/unievent/ResolveFunction.h>
#include <panda/unievent/Socks.h>
#include <panda/unievent/Stream.h>
#include <panda/unievent/Timer.h>
#include <panda/unievent/socks/SocksFilter.h>
#include <panda/string.h>
#include <panda/string_view.h>

namespace panda { namespace unievent {

constexpr bool use_cached_resolver_by_default = true; 

uint16_t find_free_port();

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

class ConnectRequest;
class TCPConnectRequest;
class TCPConnectAutoBuilder;
class ResolveRequest;
class TCP : public virtual Stream, public AllocatedObject<TCP> {
public:
    ~TCP();

    TCP(Loop* loop = Loop::default_loop(), bool cached_resolver = use_cached_resolver_by_default);

    TCP(Loop* loop, unsigned int flags, bool cached_resolver = use_cached_resolver_by_default);

    virtual void open(sock_t socket);

    virtual void bind(const sockaddr* sa, unsigned int flags = 0);

    virtual void bind(std::string_view host, std::string_view service, const addrinfo* hints = &defhints);

    virtual void connect(TCPConnectRequest* tcp_connect_request);

    virtual void connect(const sockaddr* sa, uint64_t timeout = 0);

    virtual void connect(const string& host, const string& service, uint64_t timeout = 0, const addrinfo* hints = nullptr);

    virtual TCPConnectAutoBuilder connect(); 

    virtual void reconnect(TCPConnectRequest* tcp_connect_request);

    virtual void reconnect(const sockaddr* sa, uint64_t timeout = 0);

    virtual void reconnect(const string& host, const string& service, uint64_t timeout, const addrinfo* hints = nullptr);

    void connect_internal(TCPConnectRequest* connect_request);

    using Stream::use_ssl;
    void use_ssl(const SSL_METHOD* method = nullptr) override;
    void use_socks(std::string_view host, uint16_t port = 1080, std::string_view login = "", std::string_view passw = "", bool socks_resolve = true);
    void use_socks(const SocksSP& socks);

    void tcp_nodelay (bool enable) {
        int err = uv_tcp_nodelay(&uvh, enable);
        if (err) throw CodeError(err);
    }

    void tcp_keepalive (bool enable, unsigned int delay) {
        int err = uv_tcp_keepalive(&uvh, enable, delay);
        if (err) throw CodeError(err);
    }

    void tcp_simultaneous_accepts (bool enable) {
        int err = uv_tcp_simultaneous_accepts(&uvh, enable);
        if (err) throw CodeError(err);
    }

    void getsockname (sockaddr* name, int* namelen) {
        int err = uv_tcp_getsockname(&uvh, name, namelen);
        if (err) throw CodeError(err);
    }

    void getpeername (sockaddr* name, int* namelen) {
        int err = uv_tcp_getpeername(&uvh, name, namelen);
        if (err) throw CodeError(err);
    }

#ifdef _WIN32
    void setsockopt (int level, int optname, const void* optval, int optlen) {
    	if (::setsockopt(fileno(), level, optname, (const char*)optval, optlen)) throw CodeError(WSAGetLastError());
    }
#else
    void setsockopt (int level, int optname, const void* optval, socklen_t optlen) {
    	if (::setsockopt(fileno(), level, optname, optval, optlen)) throw CodeError(-errno);
    }
#endif
    
    using Handle::set_recv_buffer_size;
    using Handle::set_send_buffer_size;

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
   
    AbstractResolverSP resolver;

protected:
    void on_handle_reinit () override;
    void _close() override;
    void init(bool cached_resolver);

private:
    uv_tcp_t uvh;
    static addrinfo defhints;
    ResolveRequestSP resolve_request;
    TimerSP connect_timer;
};

using TCPSP = iptr<TCP>;

class TCPConnectRequest : public ConnectRequest {
    friend std::ostream& operator<<(std::ostream& os, const ConnectRequest& cr);

protected:
    TCPConnectRequest(bool            reconnect,
                      const sockaddr* sa,
                      const string&   host,
                      const string&   service,
                      const addrinfo* hints,
                      uint64_t        timeout,
                      connect_fn      callback,
                      const SocksSP&  socks)
            : ConnectRequest(callback, reconnect)
            , host_(host)
            , service_(service)
            , timeout_(timeout)
            , socks_(socks) {
        _ECTOR();
        if (sa) {
            memcpy((char*)&addr_, (char*)(sa), sizeof(sa));
            resolved_ = true;
        } 

        if (hints) {
            hints_ = *hints;
        }
    }

private:
    template <class Derived> 
    struct BasicBuilder {

        BasicBuilder() { }

        Derived& concrete() { return static_cast<Derived&>(*this); }

        Derived& to(const string& host, const string& service, const addrinfo* hints = nullptr) {
            host_    = host;
            service_ = service;
            hints_   = hints;
            return concrete();
        }
        
        Derived& to(const string& host, uint16_t port, const addrinfo* hints = nullptr) {
            host_    = host;
            service_ = panda::to_string(port);
            hints_   = hints;
            return concrete();
        }

        Derived& to(const sockaddr* sa) { sa_ = sa; return concrete(); }

        Derived& timeout(uint64_t timeout) { timeout_ = timeout; return concrete(); }
        
        Derived&
        socks(std::string_view host, uint16_t port = 1080, std::string_view login = "", std::string_view passw = "", bool socks_resolve = true) {
            socks_ = new Socks(string(host), port, string(login), string(passw), socks_resolve);
            return concrete();
        }

        Derived& socks(const SocksSP& socks) { socks_ = socks; return concrete(); }
        
        Derived& callback(connect_fn callback) { callback_ = callback; return concrete(); } 
        
        Derived& reconnect(bool reconnect) { reconnect_ = reconnect; return concrete(); }

        TCPConnectRequest* build() {
            return new TCPConnectRequest(reconnect_, sa_, host_, service_, hints_, timeout_, callback_, socks_);
        }

        BasicBuilder(const BasicBuilder&) = default;
        BasicBuilder(BasicBuilder&&)      = default;

    protected:
        bool            reconnect_{false};
        const sockaddr* sa_{};
        string          host_{};
        string          service_{};
        const addrinfo* hints_{};
        uint64_t        timeout_{0};
        connect_fn      callback_{};
        SocksSP         socks_{};
    };

public:
    ~TCPConnectRequest() { _EDTOR(); }

    Handle* handle() { return static_cast<Handle*>(uvr_.handle->data); }

    struct Builder : BasicBuilder<Builder> {};

    CallbackDispatcher<connect_fptr> event;

    string           host_{};
    string           service_{};
    addrinfo         hints_{};
    bool             resolved_{false};
    sockaddr_storage addr_{};
    uint64_t         timeout_{0};
    SocksSP          socks_{};

private:
    uv_connect_t uvr_;
};

inline
std::ostream& operator<<(std::ostream& os, const TCPConnectRequest& r) {
    if(r.resolved_) {
        return os << r.host_ << ":" << r.service_;
    } else {
        return os << (sockaddr*)&r.addr_;
    }
}

using TCPConnectRequestSP = iptr<TCPConnectRequest>;

class TCPConnectAutoBuilder : public TCPConnectRequest::BasicBuilder<TCPConnectAutoBuilder> {
public:
    ~TCPConnectAutoBuilder() { tcp_->connect(this->build()); }
    TCPConnectAutoBuilder(TCP* tcp) : tcp_(tcp) {}

private:
    TCP* tcp_;
};

in_port_t find_free_port();

}}
