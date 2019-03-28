#pragma once
#include "Queue.h"
#include "Handle.h"
#include "Request.h"
#include "AddrInfo.h"
#include "backend/BackendUdp.h"

namespace panda { namespace unievent {

struct Udp : virtual Handle, panda::lib::AllocatedObject<Udp>, private backend::IUdpListener {
    struct SendRequest;
    using SendRequestSP = iptr<SendRequest>;
    using receive_fptr  = void(const UdpSP& handle, string& buf, const net::SockAddr& addr, unsigned flags, const CodeError* err);
    using receive_fn    = function<receive_fptr>;
    using send_fptr     = void(const UdpSP& handle, const CodeError* err, const SendRequestSP& req);
    using send_fn       = function<send_fptr>;
    using BackendUdp    = backend::BackendUdp;
    using Flags         = BackendUdp::Flags;
    using Membership    = BackendUdp::Membership;

    struct SendRequest : BufferRequest, lib::AllocatedObject<SendRequest>, private backend::ISendListener {
        CallbackDispatcher<send_fptr> event;
        net::SockAddr                 addr;

        using BufferRequest::BufferRequest;

        void set (Udp* h) {
            handle = h;
            BufferRequest::set(h, h->loop()->impl()->new_send_request(this));
        }

    private:
        friend Udp;
        Udp* handle;

        backend::BackendSendRequest* impl () const { return static_cast<backend::BackendSendRequest*>(_impl); }


        void exec        () override;
        void on_cancel   () override;
        void handle_send (const CodeError*) override;
    };

    buf_alloc_fn                     buf_alloc_callback;
    CallbackDispatcher<receive_fptr> receive_event;
    CallbackDispatcher<send_fptr>    send_event;

    Udp (const LoopSP& loop = Loop::default_loop(), int domain = AF_UNSPEC) : domain(domain) {
        _ECTOR();
        _init(loop, loop->impl()->new_udp(this, domain));
    }

    const HandleType& type () const override;

    string buf_alloc (size_t cap) noexcept override;

    virtual void open       (sock_t socket);
    virtual void bind       (const net::SockAddr&, unsigned flags = 0);
    virtual void bind       (string_view host, uint16_t port, const AddrInfoHints& hints = defhints, unsigned flags = 0);
    virtual void recv_start (receive_fn callback = nullptr);
    virtual void recv_stop  ();
    virtual void reset      () override;
    virtual void send       (const SendRequestSP& req);

    void send (const string& data, const net::SockAddr& sa, send_fn callback = {}) {
        auto rp = new SendRequest(data);
        rp->addr = sa;
        if (callback) rp->event.add(callback);
        send(rp);
    }

    template <class It>
    void send (It begin, It end, const net::SockAddr& sa, send_fn callback = nullptr) {
        auto rp = new SendRequest(begin, end);
        rp->addr = sa;
        if (callback) rp->event.add(callback);
        send(rp);
    }

    net::SockAddr get_sockaddr () const { return impl()->get_sockaddr(); }

    void set_membership          (std::string_view multicast_addr, std::string_view interface_addr, Membership membership);
    void set_multicast_loop      (bool on);
    void set_multicast_ttl       (int ttl);
    void set_multicast_interface (std::string_view interface_addr);
    void set_broadcast           (bool on);
    void set_ttl                 (int ttl);

    using Handle::fileno;
    using Handle::recv_buffer_size;
    using Handle::send_buffer_size;

    static const HandleType TYPE;

protected:
    virtual void on_send    (const CodeError* err, const SendRequestSP& req);
    virtual void on_receive (string& buf, const net::SockAddr& sa, unsigned flags, const CodeError* err);

    BackendUdp* impl () const { return static_cast<BackendUdp*>(_impl); }

private:
    friend SendRequest;
    int domain;
    Queue queue;
    static AddrInfoHints defhints;

    void handle_receive (string& buf, const net::SockAddr& sa, unsigned flags, const CodeError* err) override;
};

}}
