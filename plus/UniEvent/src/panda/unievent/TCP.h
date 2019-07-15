#pragma once

#include "Fwd.h"
#include "Timer.h"
#include "Stream.h"
#include "Resolver.h"

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

struct TCP : virtual Stream, AllocatedObject<TCP> {
    friend TCPConnectRequest;

    static constexpr bool USE_CACHED_RESOLVER_BY_DEFAULT = true;

    ~TCP ();
    TCP (Loop* loop = Loop::default_loop(), bool cached_resolver = USE_CACHED_RESOLVER_BY_DEFAULT);
    TCP (Loop* loop, unsigned int flags, bool cached_resolver = USE_CACHED_RESOLVER_BY_DEFAULT);

    virtual void open (sock_t socket);

    virtual void bind (const SockAddr&, unsigned int flags = 0);

    virtual void bind (string_view host, uint16_t port, const AddrInfoHintsSP& hints = default_hints());

    virtual void connect (TCPConnectRequest* tcp_connect_request);

    virtual void connect (const SockAddr& sa, uint64_t timeout = 0);

    virtual void connect (const string& host, uint16_t port, uint64_t timeout = 0, const AddrInfoHintsSP& hints = default_hints());

    virtual TCPConnectAutoBuilder connect (); 

    virtual void reconnect (TCPConnectRequest* tcp_connect_request);

    virtual void reconnect (const SockAddr& sa, uint64_t timeout = 0);

    virtual void reconnect (const string& host, uint16_t port, uint64_t timeout, const AddrInfoHintsSP& hints = default_hints());

    void do_connect (TCPConnectRequest* connect_request);

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

    StreamSP on_create_connection () override;
    ResolverSP resolver() { return loop()->resolver(); }

    using Handle::set_recv_buffer_size;
    using Handle::set_send_buffer_size;

protected:
    void on_handle_reinit () override;
    void _close () override;

public:    
    bool cached_resolver;
   
private:
    uv_tcp_t uvh;
    TimerSP connect_timer;
    static AddrInfoHintsSP default_hints();
    ResolveRequestSP resolve_request;
};

std::ostream& operator<< (std::ostream&, const TCP&);

struct TCPConnectRequest : ConnectRequest {
    template <class Derived> 
    struct BasicBuilder {

        BasicBuilder () { }

        Derived& concrete () { return static_cast<Derived&>(*this); }
        
        Derived& to (const string& host, uint16_t port, const AddrInfoHintsSP& hints = TCP::default_hints()) {
            _host  = host;
            _port  = port;
            _hints = hints;
            return concrete();
        }

        Derived& to (const SockAddr& sa) { _sa = sa; return concrete(); }

        Derived& timeout (uint64_t timeout) { _timeout = timeout; return concrete(); }
                
        Derived& callback (connect_fn callback) { _callback = callback; return concrete(); } 
        
        Derived& reconnect (bool reconnect) { _reconnect = reconnect; return concrete(); }

        TCPConnectRequest* build () {
            return new TCPConnectRequest(_reconnect, _sa, _host, _port, _hints, _timeout, _callback);
        }

        BasicBuilder (const BasicBuilder&) = default;
        BasicBuilder (BasicBuilder&&)      = default;

    protected:
        bool            _reconnect = false;
        SockAddr        _sa;
        string          _host;
        uint16_t        _port;
        AddrInfoHintsSP _hints   = TCP::default_hints();
        uint64_t        _timeout = 0;
        connect_fn      _callback;
    };

    Handle* handle() { return static_cast<Handle*>(uvr.handle->data); }

    struct Builder : BasicBuilder<Builder> {};

    CallbackDispatcher<connect_fptr> event;

    string          host;
    uint16_t        port;
    AddrInfoHintsSP hints;
    SockAddr        sa;
    uint64_t        timeout = 0;

protected:
    TCPConnectRequest(
        bool reconnect, const SockAddr& sa, const string& host, uint16_t port, const AddrInfoHintsSP& hints, uint64_t timeout, connect_fn callback)
            : ConnectRequest(callback, reconnect), host(host), port(port), hints(hints), sa(sa), timeout(timeout) {}

private:
    friend std::ostream& operator<< (std::ostream& os, const ConnectRequest& cr);

    uv_connect_t uvr;
};

struct TCPConnectAutoBuilder : TCPConnectRequest::BasicBuilder<TCPConnectAutoBuilder> {
    TCPConnectAutoBuilder (TCP* tcp) : tcp(tcp) {}
    ~TCPConnectAutoBuilder () { tcp->connect(this->build()); }
private:
    TCP* tcp;
};

}}
