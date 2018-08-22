#include <panda/unievent/UDP.h>
using namespace panda::unievent;

static addrinfo _init_defhints () {
    addrinfo ret;
    memset(&ret, 0, sizeof(ret));
    ret.ai_family   = PF_UNSPEC;
    ret.ai_socktype = SOCK_DGRAM;
    ret.ai_flags    = AI_PASSIVE;
    return ret;
}

addrinfo UDP::defhints = _init_defhints();

void UDP::uvx_on_receive (uv_udp_t* handle, ssize_t nread, const uv_buf_t* uvbuf, const sockaddr* addr, unsigned flags) {
    UDP* h = hcast<UDP*>(handle);

    string* buf_ptr = (string*)(uvbuf->base + uvbuf->len);
    string buf = *buf_ptr;
    buf_ptr->~string();

    if (!nread && !addr) return; // nothing to read

    UDPError err(nread < 0 ? nread : 0);
    buf.length(nread > 0 ? nread : 0); // set real buf len
    h->call_on_receive(buf, addr, flags, err);
}

void UDP::uvx_on_send (uv_udp_send_t* uvreq, int status) {
    SendRequest* r = rcast<SendRequest*>(uvreq);
    UDP* h = hcast<UDP*>(uvreq->handle);
    UDPError err(status < 0 ? status : 0);
    h->call_on_send(err, r);
}

void UDP::open (sock_t socket) {
    int err = uv_udp_open(&uvh, socket);
    if (err) throw UDPError(err);
}

void UDP::bind (const sockaddr* sa, unsigned flags) {
    int err = uv_udp_bind(&uvh, sa, flags);
    if (err) throw UDPError(err);
}

void UDP::bind (string_view host, string_view service, const addrinfo* hints, unsigned flags) {
    if (!hints) hints = &defhints;

    char host_cstr[host.length()+1];
    std::memcpy(host_cstr, host.data(), host.length());
    host_cstr[host.length()] = 0;

    char service_cstr[service.length()+1];
    std::memcpy(service_cstr, service.data(), service.length());
    service_cstr[service.length()] = 0;

    addrinfo* res;
    int syserr = getaddrinfo(host_cstr, service_cstr, hints, &res);
    if (syserr) throw TCPError(_err_gai2uv(syserr));

    try { bind(res->ai_addr, flags); }
    catch (...) {
        freeaddrinfo(res);
        throw;
    }
    freeaddrinfo(res);
}

void UDP::recv_start (receive_fn callback) {
    if (callback) receive_event.add(callback);
    int err = uv_udp_recv_start(&uvh, Handle::uvx_on_buf_alloc, uvx_on_receive);
    if (err) throw UDPError(err);
}

void UDP::recv_stop () {
    int err = uv_udp_recv_stop(&uvh);
    if (err) throw UDPError(err);
}

void UDP::send (SendRequest* req, const sockaddr* sa) {
    assert(req);
    req->retain();
    req->event.add(send_event);

    auto nbufs = req->bufs.size();
    uv_buf_t uvbufs[nbufs];
    uv_buf_t* ptr = uvbufs;
    for (const auto& str : req->bufs) {
        ptr->base = (char*)str.data();
        ptr->len  = str.length();
        ++ptr;
    }

    int err = uv_udp_send(_pex_(req), &uvh, uvbufs, nbufs, sa, uvx_on_send);
    if (err) {
        req->release();
        throw UDPError(err);
    }
    retain();
}

void UDP::reset () { close_reinit(); }

void UDP::on_handle_reinit () {
    int err = uv_udp_init(uvh.loop, &uvh);
    if (err) throw UDPError(err);
    Handle::on_handle_reinit();
}

void UDP::on_receive (const string& buf, const sockaddr* addr, unsigned flags, const UDPError& err) {
    if (receive_event.has_listeners()) receive_event(this, buf, addr, flags, err);
    else throw ImplRequiredError("UDP::on_receive");
}

void UDP::on_send (const UDPError& err, SendRequest* r) {
    r->event(this, err, r);
}
