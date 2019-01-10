#include "UDP.h"
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

    int status = 0;
    if (nread < 0) {
        status = nread;
        nread = 0;
    }

    buf.length(nread); // set real buf len
    h->call_on_receive(buf, addr, flags, CodeError(status));
}

void UDP::uvx_on_send (uv_udp_send_t* uvreq, int status) {
    SendRequest* r = rcast<SendRequest*>(uvreq);
    UDP* h = hcast<UDP*>(uvreq->handle);
    h->call_on_send(CodeError(status), r);
}

void UDP::open (sock_t socket) {
    int err = uv_udp_open(&uvh, socket);
    if (err) throw CodeError(err);
}

void UDP::bind (const SockAddr& sa, unsigned flags) {
    int err = uv_udp_bind(&uvh, sa.get(), flags);
    if (err) throw CodeError(err);
}

void UDP::bind (string_view host, string_view service, const addrinfo* hints, unsigned flags) {
    if (!hints) hints = &defhints;

    PEXS_NULL_TERMINATE(host, host_cstr);
    PEXS_NULL_TERMINATE(service, service_cstr);

    addrinfo* res;
    int syserr = getaddrinfo(host_cstr, service_cstr, hints, &res);
    if (syserr) throw CodeError(_err_gai2uv(syserr));

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
    if (err) throw CodeError(err);
}

void UDP::recv_stop () {
    int err = uv_udp_recv_stop(&uvh);
    if (err) throw CodeError(err);
}

void UDP::send (SendRequest* req, const SockAddr& sa) {
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

    int err = uv_udp_send(_pex_(req), &uvh, uvbufs, nbufs, sa.get(), uvx_on_send);
    if (err) {
        req->release();
        throw CodeError(err);
    }
    retain();
}

void UDP::reset () { close_reinit(); }

void UDP::on_handle_reinit () {
    int err = uv_udp_init(uvh.loop, &uvh);
    if (err) throw CodeError(err);
    Handle::on_handle_reinit();
}

void UDP::on_receive (string& buf, const SockAddr& sa, unsigned flags, const CodeError* err) {
    if (receive_event.has_listeners()) receive_event(this, buf, sa, flags, err);
    else throw ImplRequiredError("UDP::on_receive");
}

void UDP::on_send (const CodeError* err, SendRequest* r) {
    r->event(this, err, r);
}