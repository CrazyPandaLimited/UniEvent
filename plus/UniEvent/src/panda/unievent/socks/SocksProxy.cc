#include "SocksProxy.h"
/*
#include <panda/event/CachedResolver.h>
#include <panda/event/Debug.h>
#include <panda/event/StreamFilter.h>
#include <panda/event/TCP.h>
#include <panda/event/Timer.h>
#include <panda/string.h>

#include <vector>

namespace panda { namespace event { namespace socks {

namespace {
#define MACHINE_DATA
#include "SocksProxyParser.cc"
} // namespace

SocksProxy::~SocksProxy() { _EDEBUGTHIS("dtor"); }

SocksProxy::SocksProxy(TCP* tcp, SocksSP socks)
        : tcp_(tcp)
        , socks_(socks) {
    _EDEBUGTHIS("ctor");
    init_parser();
}

void SocksProxy::init_parser() {
    atyp   = 0;
    rep    = 0;
    noauth = false;
}

void SocksProxy::uvx_on_connect(uv_connect_t* uvreq, int status) {
    _EDEBUG("uvx_on_connect");
    SocksConnectRequest* r = rcast<SocksConnectRequest*>(uvreq);
    TCP*                 h = hcast<TCP*>(uvreq->handle);
    StreamError          err(status < 0 ? status : 0);
    r->event(h, err, r);
}

void SocksProxy::uvx_on_close(uv_handle_t* uvh) {
    _EDEBUG("uvx_on_close");
    TCP*             h     = hcast<TCP*>(uvh);
    iptr<SocksProxy> proxy = h->socks_proxy;
    proxy->do_close();
}

void SocksProxy::uvx_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* uvbuf) {
    _EDEBUG("uvx_on_read, got %d bytes", (int)nread);

    // in some cases of eof it may be no buffer
    string buf;
    if (uvbuf->base) {
        string* buf_ptr = (string*)(uvbuf->base + uvbuf->len);
        buf             = *buf_ptr;
        buf_ptr->~string();
    }

    TCP* h = hcast<TCP*>(stream);

    iptr<SocksProxy> proxy = h->socks_proxy;

    if (proxy->state_ == State::canceled) {
        return;
    }

    StreamError err(nread < 0 ? nread : 0);
    if (err.code() == ERRNO_EOF) {
        _EDEBUG("eof %d", (int)proxy->state_);
        if (proxy->state_ == State::parsing || proxy->state_ == State::handshake_reply || proxy->state_ == State::auth_reply || proxy->state_ == State::connect_reply) {
            proxy->do_transition(State::eof);
        }
        return;
    }

    if (nread == 0) {
        // UV just wants to release the buf
        _EDEBUG("release buf");
        return;
    }

    if (nread < 0) {
        _EDEBUG("error");
        proxy->do_transition(State::error);
        return;
    }

    buf.length(nread > 0 ? nread : 0); // set real buf len

    _EDUMP(buf, (int)buf.length(), 100);

    // pointer to current buffer
    const char* buffer_ptr = buf.data();
    // start parsing from the beginning pointer
    const char* p = buffer_ptr;
    // to the end pointer
    const char* pe = buffer_ptr + buf.size();

    int& cs = proxy->cs;

    // select reply parser by our state
    switch (proxy->state_) {
        case State::handshake_reply:
            cs = socks5_client_parser_en_negotiate_reply;
            break;
        case State::auth_reply:
            cs = socks5_client_parser_en_auth_reply;
            break;
        case State::connect_reply:
            cs = socks5_client_parser_en_connect_reply;
            break;
        case State::parsing:
            break;
        case State::terminal:
            _EDEBUG("wont parse, in a terminal state");
            return;
        default:
            _EDEBUG("bad state, len: %d", int(p - buffer_ptr));
            proxy->state_ = State::error;
            return;
    }

    proxy->state_ = State::parsing;

// generated parser logic
#define MACHINE_EXEC
#include "SocksProxyParser.cc"

    proxy->print_pointers();

    if (proxy->state_ == State::error) {
        _EDEBUG("parser exiting in error state on pos: %d", int(p - buffer_ptr));
    } else if (proxy->state_ != State::parsing) {
        _EDEBUG("parser ok");
        proxy->init_parser();
    }
}

void SocksProxy::uvx_on_write(uv_write_t* uvreq, int status) {
    _EDEBUG("uvx_on_write %d", status);
    SocksWriteRequest* r = rcast<SocksWriteRequest*>(uvreq);
    TCP*               h = hcast<TCP*>(uvreq->handle);
    StreamError        err(status < 0 ? status : 0);
    r->event(h, err, r);
}

bool SocksProxy::start(const sockaddr* sa, ConnectRequestSP connect_request) {
    _EDEBUGTHIS("start %p", connect_request.get());

    host_resolved_ = true;

    memcpy((char*)&socks_addr_, (char*)(sa), sizeof(sa));

    connect_request_ = connect_request;

    tcp_->retain();

    do_transition(State::connect_proxy);

    return state_ != State::error;
}

bool SocksProxy::start(const string& host, const string& service, const addrinfo* hints, ConnectRequestSP connect_request) {
    _EDEBUGTHIS("start %p", connect_request.get());

    host_resolved_ = false;

    int port;
    std::from_chars(service.data(), service.data() + service.size(), port);

    if (port <= 0 || port > 65535) {
        throw Error("Bad port number");
    }

    host_ = host;
    port_ = port;

    if (hints) {
        hints_ = *hints;
    }

    connect_request_ = connect_request;

    tcp_->retain();

    do_transition(State::resolve_proxy);

    return state_ != State::error;
}

void SocksProxy::cancel() {
    _EDEBUGTHIS("cancel state: %d", (int)state_);

    switch (state_) {
        case State::resolving_proxy:
        case State::resolving_host:
            _EDEBUGTHIS("cancel resolve");
            resolve_request_->cancel();
            break;
        case State::connecting_proxy:
            _EDEBUGTHIS("cancel connect proxy");
            state_ = State::canceled;
            break;
        case State::handshake_write:
        case State::auth_write:
        case State::connect_write: {
            state_ = State::canceled;
            break;
        }
        case State::handshake_reply:
        case State::auth_reply:
        case State::connect_reply: {
            _EDEBUGTHIS("cancel reply");
            state_ = State::canceled;
            StreamError err(ERRNO_SOCKS);
            tcp_->call_on_connect(err, connect_request_, false);
            tcp_->release();
            break;
        }
        default:
            break;
    }
}

void SocksProxy::do_write(SocksWriteRequest* req) {
    _EDEBUGTHIS("do_write");
    uv_buf_t uvbufs[] = {{.base = (char*)req->buffer_.data(), .len = req->buffer_.length()}};
    int      err      = uv_write(_pex_(req), _pex_(tcp_), uvbufs, 1, uvx_on_write);
    if (err) {
        _EDEBUGTHIS("do_write error: %d", err);
        do_transition(State::error);
        return;
    }
}

void SocksProxy::do_transition(State s) {
    _EDEBUGTHIS("do_transition %d -> %d", (int)state_, (int)s);
    state_ = s;
    switch (state_) {
        case State::resolve_proxy:
            do_resolve_proxy();
            break;
        case State::connect_proxy:
            do_connect_proxy();
            break;
        case State::handshake:
            do_handshake();
            break;
        case State::auth:
            do_auth();
            break;
        case State::resolve_host:
            do_resolve_host();
            break;
        case State::connect:
            do_connect();
            break;
        case State::connected:
            do_connected();
            break;
        case State::error:
            do_error();
            break;
        case State::eof:
            do_eof();
            break;
        default:
            // ignores all the others
            break;
    }
}

void SocksProxy::do_resolve_proxy() {
    _EDEBUGTHIS("do_resolve_proxy");

    CachedResolver*                           resolver = get_thread_local_cached_resolver();
    CachedResolver::CacheType::const_iterator address_pos;
    bool                                      found;
    std::tie(address_pos, found) = resolver->find(socks_->host, panda::to_string(socks_->port));
    if (found) {
        _EDEBUGTHIS("in cache");
        auto ai_addr = address_pos->second->next()->ai_addr;
        memcpy((char*)&socks_addr_, (char*)ai_addr, sizeof(ai_addr));
        do_transition(State::connect_proxy);
        return;
    }

    resolve_request_ = resolver->resolve_async(tcp_->loop(), socks_->host, panda::to_string(socks_->port), nullptr,
                                               [=](addrinfo* res, const ResolveError& err, bool) {
                                                   _EDEBUGTHIS("resolved err:%d", err.code());
                                                   if (err) {
                                                       do_transition(State::error);
                                                       return;
                                                   }

                                                   memcpy((char*)&socks_addr_, (char*)res->ai_addr, sizeof(res->ai_addr));

                                                   if (socks_->socks_resolve || host_resolved_) {
                                                       // we have resolved the host or proxy will resolve it for us
                                                       do_transition(State::connect_proxy);
                                                   } else {
                                                       // we will resolve the host ourselves
                                                       do_transition(State::resolve_host);
                                                   }
                                               });
    state(State::resolving_proxy);
}

void SocksProxy::do_connect_proxy() {
    _EDEBUGTHIS("do_connect_proxy");
    socks_connect_request_ = new SocksConnectRequest([=](TCP*, const StreamError& err, SocksConnectRequest*) {
        _EDEBUGTHIS("connect callback, err: %d", err.code());
        if (this->state_ == State::canceled) {
            do_transition(State::error);
            return;
        }

        if (err) {
            do_transition(State::error);
            return;
        }

        do_transition(State::handshake);
    });

    _pex_(socks_connect_request_)->handle = _pex_(tcp_);

    int uverr = uv_tcp_connect(_pex_(socks_connect_request_), &tcp_->uvh, (sockaddr*)&socks_addr_, SocksProxy::uvx_on_connect);
    if (uverr) {
        _EDEBUG("Connection error");
        do_transition(State::error);
        return;
    }

    state(State::connecting_proxy);
}

void SocksProxy::do_handshake() {
    _EDEBUGTHIS("do_handshake");
    SocksHandshakeRequest* socks_handshake_request = new SocksHandshakeRequest(
        [=](TCP* handle, const StreamError& err, SocksWriteRequest*) {
            _EDEBUGTHIS("handshake callback");
            if (this->state_ == State::canceled) {
                do_transition(State::error);
                return;
            }

            if (err) {
                do_transition(State::error);
                return;
            }

            int uverr = uv_read_start(_pex_(handle), Handle::uvx_on_buf_alloc, uvx_on_read);
            if (uverr) {
                _EDEBUGTHIS("leaving");
                do_transition(State::error);
            }
            state(State::handshake_reply);
        },
        socks_);

    current_request_ = socks_handshake_request;
    state(State::handshake_write);
    do_write(socks_handshake_request);
}

void SocksProxy::do_auth() {
    _EDEBUGTHIS("do_auth");
    SocksAuthRequest* socks_auth_request = new SocksAuthRequest(
        [=](TCP* handle, const StreamError& err, SocksWriteRequest*) {
            _EDEBUGTHIS("auth callback");
            if (this->state_ == State::canceled) {
                do_transition(State::error);
                return;
            }

            if (err) {
                do_transition(State::error);
                return;
            }

            int uverr = uv_read_start(_pex_(handle), Handle::uvx_on_buf_alloc, uvx_on_read);
            if (uverr) {
                _EDEBUGTHIS("leaving");
                do_transition(State::error);
            }
            state(State::auth_reply);
        },
        socks_);

    current_request_ = socks_auth_request;
    state(State::auth_write);
    do_write(socks_auth_request);
}

void SocksProxy::do_resolve_host() {
    _EDEBUGTHIS("do_resolve_host");

    CachedResolver*                           resolver = get_thread_local_cached_resolver();
    CachedResolver::CacheType::const_iterator address_pos;
    bool                                      found;
    std::tie(address_pos, found) = resolver->find(host_, panda::to_string(port_), &hints_);
    if (found) {
        _EDEBUGTHIS("in cache");
        auto ai_addr = address_pos->second->next()->ai_addr;
        memcpy((char*)&addr_, (char*)ai_addr, sizeof(ai_addr));
        do_transition(State::connect_proxy);
        return;
    }

    resolve_request_ =
        resolver->resolve_async(tcp_->loop(), host_, panda::to_string(port_), nullptr, [=](addrinfo* res, const ResolveError& err, bool) {
            _EDEBUGTHIS("resolved err:%d", err.code());
            if (err) {
                do_transition(State::error);
                return;
            }

            host_resolved_ = true;
            memcpy((char*)&addr_, (char*)res->ai_addr, sizeof(res->ai_addr));
            do_transition(State::connect_proxy);
        });

    state(State::resolving_host);
}

void SocksProxy::do_connect() {
    _EDEBUGTHIS("do_connect");
    SocksCommandConnectRequest* socks_command_connect_request;
    if (host_resolved_) {
        socks_command_connect_request = new SocksCommandConnectRequest(
            [=](TCP* handle, const StreamError& err, SocksWriteRequest*) {
                _EDEBUGTHIS("command connect callback %d", err.code());
                if (this->state_ == State::canceled) {
                    do_transition(State::error);
                    return;
                }

                if (err) {
                    do_transition(State::error);
                    return;
                }

                state(State::connect_reply);

                int uverr = uv_read_start(_pex_(handle), Handle::uvx_on_buf_alloc, uvx_on_read);
                if (uverr) {
                    do_transition(State::error);
                    return;
                }
            },
            (sockaddr*)&addr_);
    } else {
        socks_command_connect_request = new SocksCommandConnectRequest(
            [=](TCP* handle, const StreamError& err, SocksWriteRequest*) {
                _EDEBUGTHIS("command connect callback %d", err.code());
                if (this->state_ == State::canceled) {
                    do_transition(State::error);
                    return;
                }

                if (err) {
                    _EDEBUGTHIS("leaving");
                    do_transition(State::error);
                    return;
                }

                state(State::connect_reply);

                int uverr = uv_read_start(_pex_(handle), Handle::uvx_on_buf_alloc, uvx_on_read);
                if (uverr) {
                    _EDEBUGTHIS("leaving");
                    do_transition(State::error);
                    return;
                }
            },
            host_, port_);
    }

    current_request_ = socks_command_connect_request;

    state(State::connect_write);

    do_write(socks_command_connect_request);
}

void SocksProxy::do_connected() {
    _EDEBUGTHIS("do_connected");

    state(State::terminal);

    // restore those who wanted read before we started proxy
    if (tcp_->want_read()) {
        tcp_->read_start();
    }

    StreamError err(0);
    tcp_->filters() ? tcp_->filters()->on_connect(err, connect_request_) : tcp_->call_on_connect(err, connect_request_, true);
    tcp_->release();
}

void SocksProxy::do_error() {
    _EDEBUGTHIS("do_error");

    state(State::terminal);

    if (tcp_->want_read()) {
        tcp_->read_start();
    }

    StreamError err(ERRNO_SOCKS);
    tcp_->call_on_connect(err, connect_request_, false);
    tcp_->release();
}

void SocksProxy::do_close() {
    _EDEBUGTHIS("do_close");

    state(State::terminal);

    tcp_->disconnect();
}

void SocksProxy::do_eof() {
    _EDEBUGTHIS("do_eof");

    state(State::terminal);

    if (tcp_->want_read()) {
        tcp_->read_start();
    }

    StreamError err(ERRNO_SOCKS);
    tcp_->call_on_connect(err, connect_request_, true);
    tcp_->release();
}

void SocksProxy::print_pointers() const {
    _EDEBUGTHIS("socks:%d tcp:%d connect:%d resolve:%d current:%d", refcnt(), tcp_->refcnt(), connect_request_->refcnt(), resolve_request_->refcnt(),
                current_request_->refcnt());
}

SocksConnectRequest::SocksConnectRequest(connect_fn callback) {
    _EDEBUGTHIS("ctor");
    event.add(callback);
    _init(&uvr_);
}

SocksConnectRequest::~SocksConnectRequest() { _EDEBUGTHIS("dtor"); }

}}} // namespace panda::event::socks
*/
