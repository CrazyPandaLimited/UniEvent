#pragma once
#include "Queue.h"
#include "Handle.h"
#include "Request.h"
#include "AddrInfo.h"
#include "backend/BackendUdp.h"

namespace panda { namespace unievent {

struct Udp : virtual Handle, panda::lib::AllocatedObject<Udp>, private backend::IUdpListener {
    using receive_fptr  = void(const UdpSP& handle, string& buf, const net::SockAddr& addr, unsigned flags, const CodeError& err);
    using receive_fn    = function<receive_fptr>;
    using send_fptr     = void(const UdpSP& handle, const CodeError& err, const SendRequestSP& req);
    using send_fn       = function<send_fptr>;
    using BackendUdp    = backend::BackendUdp;
    using Flags         = BackendUdp::Flags;
    using Membership    = BackendUdp::Membership;

    buf_alloc_fn                     buf_alloc_callback;
    CallbackDispatcher<receive_fptr> receive_event;
    CallbackDispatcher<send_fptr>    send_event;

    Udp (const LoopSP& loop = Loop::default_loop(), int domain = AF_UNSPEC) : domain(domain) {
        _ECTOR();
        _init(loop, loop->impl()->new_udp(this, domain));
    }

    const HandleType& type () const override;

    string buf_alloc (size_t cap) noexcept override;

    virtual void open       (sock_t socket, Ownership = Ownership::TRANSFER);
    virtual void bind       (const net::SockAddr&, unsigned flags = 0);
    virtual void bind       (string_view host, uint16_t port, const AddrInfoHints& hints = defhints, unsigned flags = 0);
    virtual void connect    (const net::SockAddr&);
    virtual void connect    (string_view host, uint16_t port, const AddrInfoHints& hints = defhints);
    virtual void recv_start (receive_fn callback = nullptr);
    virtual void recv_stop  ();
    virtual void reset      () override;
    virtual void clear      () override;
    virtual void send       (const SendRequestSP& req);
    /*INL*/ void send       (const string& data, const net::SockAddr& sa, send_fn callback = {});
    template <class It>
    /*INL*/ void send       (It begin, It end, const net::SockAddr& sa, send_fn callback = {});

    optional<fh_t> fileno () const { return _impl ? impl()->fileno() : optional<fh_t>(); }

    net::SockAddr sockaddr () const { return impl()->sockaddr(); }
    net::SockAddr peeraddr () const { return impl()->peeraddr(); }

    int  recv_buffer_size () const    { return impl()->recv_buffer_size(); }
    void recv_buffer_size (int value) { impl()->recv_buffer_size(value); }
    int  send_buffer_size () const    { return impl()->send_buffer_size(); }
    void send_buffer_size (int value) { impl()->send_buffer_size(value); }

    void set_membership          (std::string_view multicast_addr, std::string_view interface_addr, Membership membership);
    void set_multicast_loop      (bool on);
    void set_multicast_ttl       (int ttl);
    void set_multicast_interface (std::string_view interface_addr);
    void set_broadcast           (bool on);
    void set_ttl                 (int ttl);

    static const HandleType TYPE;

protected:
    virtual void on_send    (const CodeError& err, const SendRequestSP& req);
    virtual void on_receive (string& buf, const net::SockAddr& sa, unsigned flags, const CodeError& err);

private:
    friend SendRequest;
    static AddrInfoHints defhints;

    int   domain;
    Queue queue;

    BackendUdp* impl () const { return static_cast<BackendUdp*>(Handle::impl()); }

    BackendHandle* new_impl () override;

    void handle_receive (string& buf, const net::SockAddr& sa, unsigned flags, const CodeError& err) override;
};


struct SendRequest : Request, lib::AllocatedObject<SendRequest> {
    CallbackDispatcher<Udp::send_fptr> event;
    net::SockAddr                      addr;
    std::vector<string>                bufs;

    SendRequest () {}

    SendRequest (const string& data) {
        bufs.push_back(data);
    }

    template <class It>
    SendRequest (It begin, It end) {
        bufs.reserve(end - begin);
        for (; begin != end; ++begin) bufs.push_back(*begin);
    }

private:
    friend Udp;
    Udp* handle;

    void set (Udp* h) {
        handle = h;
        Request::set(h);
    }

    backend::BackendSendRequest* impl () {
        if (!_impl) _impl = handle->impl()->new_send_request(this);
        return static_cast<backend::BackendSendRequest*>(_impl);
    }

    void exec         () override;
    void handle_event (const CodeError&) override;
};


inline void Udp::send (const string& data, const net::SockAddr& sa, send_fn callback) {
    auto rp = new SendRequest(data);
    rp->addr = sa;
    if (callback) rp->event.add(callback);
    send(rp);
}

template <class It>
inline void Udp::send (It begin, It end, const net::SockAddr& sa, send_fn callback) {
    auto rp = new SendRequest(begin, end);
    rp->addr = sa;
    if (callback) rp->event.add(callback);
    send(rp);
}

}}
