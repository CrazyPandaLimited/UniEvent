#pragma once

#include "Fwd.h"
#include "Timer.h"
#include "Socks.h"
#include "Stream.h"
#include "ResolveFunction.h"
#include "socks/SocksFilter.h"

#include <ostream>
#include <panda/string.h>
#include <panda/string_view.h>
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent {

using panda::net::SockAddr;

constexpr bool use_cached_resolver_by_default = true;

struct addrinfo_deleter {
    void operator() (addrinfo* ptr) {
        if (ptr != nullptr) freeaddrinfo(ptr);
    }
};

using addrinfo_keeper = std::unique_ptr<addrinfo, addrinfo_deleter>;

struct ConnectRequest; struct TCPConnectRequest; struct TCPConnectAutoBuilder;

struct TCP : virtual Stream, AllocatedObject<TCP> {
    friend TCPConnectRequest;

    ~TCP();

    TCP(Loop* loop = Loop::default_loop(), bool cached_resolver = use_cached_resolver_by_default);

    TCP(Loop* loop, unsigned int flags, bool cached_resolver = use_cached_resolver_by_default);

    virtual void open(sock_t socket);

    virtual void bind(const SockAddr&, unsigned int flags = 0);

    virtual void bind(std::string_view host, std::string_view service, const AddrInfoHintsSP& hints = default_hints);

    virtual void connect(TCPConnectRequest* tcp_connect_request);

    virtual void connect(const SockAddr& sa, uint64_t timeout = 0);

    virtual void connect(const string& host, const string& service, uint64_t timeout = 0, const AddrInfoHintsSP& hints = default_hints);

    virtual TCPConnectAutoBuilder connect(); 

    virtual void reconnect(TCPConnectRequest* tcp_connect_request);

    virtual void reconnect(const SockAddr& sa, uint64_t timeout = 0);

    virtual void reconnect(const string& host, const string& service, uint64_t timeout, const AddrInfoHintsSP& hints = default_hints);

    void do_connect(TCPConnectRequest* connect_request);

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

    SockAddr get_sockaddr () const {
        SockAddr ret;
        int sz = sizeof(ret);
        int err = uv_tcp_getsockname(&uvh, ret.get(), &sz);
        if (err) {
            if (err == ERRNO_EINVAL || err == ERRNO_ENOTCONN) return {};
            throw CodeError(err);
        }
        return ret;
    }

    SockAddr get_peer_sockaddr () const {
        SockAddr ret;
        int sz = sizeof(ret);
        int err = uv_tcp_getpeername(&uvh, ret.get(), &sz);
        if (err) {
            if (err == ERRNO_EINVAL || err == ERRNO_ENOTCONN) return {};
            throw CodeError(err);
        }
        return ret;
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
   
    ResolverSP resolver;

protected:
    void on_handle_reinit () override;
    void _close() override;
    void init(bool cached_resolver);

private:
    uv_tcp_t uvh;
    TimerSP connect_timer;
    static AddrInfoHintsSP default_hints;
};

std::ostream& operator<< (std::ostream&, const TCP&);

struct TCPConnectRequest : ConnectRequest {
    template <class Derived> 
    struct BasicBuilder {

        BasicBuilder() { }

        Derived& concrete() { return static_cast<Derived&>(*this); }

        Derived& to(const string& host, const string& service, const AddrInfoHintsSP& hints = TCP::default_hints) {
            host_    = host;
            service_ = service;
            hints_   = hints;
            return concrete();
        }
        
        Derived& to(const string& host, uint16_t port, const AddrInfoHintsSP& hints = TCP::default_hints) {
            host_    = host;
            service_ = panda::to_string(port);
            hints_   = hints;
            return concrete();
        }

        Derived& to(const SockAddr& sa) { sa_ = sa; return concrete(); }

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
        bool            reconnect_ = false;
        SockAddr        sa_;
        string          host_;
        string          service_;
        AddrInfoHintsSP hints_   = TCP::default_hints;
        uint64_t        timeout_ = 0;
        connect_fn      callback_;
        SocksSP         socks_;
    };


    ~TCPConnectRequest() { _EDTOR(); }

    Handle* handle() { return static_cast<Handle*>(uvr_.handle->data); }

    struct Builder : BasicBuilder<Builder> {};

    CallbackDispatcher<connect_fptr> event;

    string          host_;
    string          service_;
    AddrInfoHintsSP hints_;
    SockAddr        sa_;
    uint64_t        timeout_ = 0;
    SocksSP         socks_;

protected:
    TCPConnectRequest(bool                   reconnect,
                      const SockAddr&        sa,
                      const string&          host,
                      const string&          service,
                      const AddrInfoHintsSP& hints,
                      uint64_t               timeout,
                      connect_fn             callback,
                      const SocksSP&         socks)
            : ConnectRequest(callback, reconnect)
            , host_(host)
            , service_(service)
            , hints_(hints)
            , sa_(sa)
            , timeout_(timeout)
            , socks_(socks)
    {
        _ECTOR();
    }

private:
    friend std::ostream& operator<< (std::ostream& os, const ConnectRequest& cr);

    uv_connect_t uvr_;
};

struct TCPConnectAutoBuilder : TCPConnectRequest::BasicBuilder<TCPConnectAutoBuilder> {
    ~TCPConnectAutoBuilder() { tcp_->connect(this->build()); }
    TCPConnectAutoBuilder(TCP* tcp) : tcp_(tcp) {}

private:
    TCP* tcp_;
};

}}
