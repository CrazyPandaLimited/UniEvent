#include "SocksFilter.h"

#include <panda/unievent/Debug.h>
#include <panda/unievent/Resolver.h>
#include <panda/unievent/Stream.h>
#include <panda/unievent/StreamFilter.h>
#include <panda/unievent/TCP.h>
#include <panda/unievent/Timer.h>
#include <panda/string.h>

#include <vector>

namespace panda { namespace unievent { namespace socks {

const char* SocksFilter::TYPE = "SOCKS";

namespace {
#define MACHINE_DATA
#include "SocksParser.cc"
} // namespace

SocksFilter::~SocksFilter() { _EDTOR(); }

SocksFilter::SocksFilter(Stream* stream, SocksSP socks) : StreamFilter(stream, TYPE), socks_(socks), state_(State::initial) {
    _ECTOR();
    init_parser();
}

void SocksFilter::init_parser() {
    atyp   = 0;
    rep    = 0;
    noauth = false;
}

void SocksFilter::on_connection(StreamSP stream, const CodeError* err) {
    _EDEBUGTHIS("on_connection");
    // socks is a client only filter, so disable for incoming connections
    state(State::terminal);
    NextFilter::on_connection(stream, err);
}

void SocksFilter::connect(ConnectRequest* connect_request) {
    _EDEBUGTHIS("connect request: %p, state: %d", connect_request, (int)state_);
    if (state_ == State::terminal) {
        NextFilter::connect(connect_request);
        return;
    }

    connect_request_ = static_cast<TCPConnectRequest*>(connect_request);

    if (connect_request_->sa_) {
        sa_ = connect_request_->sa_;
    } else {
        int port = stoi(connect_request_->service_);
        if (port <= 0 || port > 65535) {
            throw Error("Bad port number");
        }

        host_  = connect_request_->host_;
        port_  = port;
        hints_ = connect_request_->hints_;
    }

    socks_connect_request_ = TCPConnectRequest::Builder().to(socks_->host, socks_->port).build();

    handle->attach(socks_connect_request_);

    state(State::connecting_proxy);

    NextFilter::connect(socks_connect_request_);
}

void SocksFilter::on_connect(const CodeError* err, ConnectRequest* connect_request) {
    _EDEBUGTHIS("on_connect, err: %d, state: %d", err ? err->code() : 0, (int)state_);
    if (state_ == State::terminal) {
        NextFilter::on_connect(err, connect_request);
        return;
    }
    
    if (err) {
        do_error(err);
        return;
    }

    auto read_err = temp_read_start();
    if (read_err) {
        do_error(read_err);
        return;
    }

    if (socks_->socks_resolve || sa_) {
        // we have resolved the host or proxy will resolve it for us
        do_handshake();
    } else {
        // we will resolve the host ourselves
        do_resolve();
    }
}

void SocksFilter::write(WriteRequest* write_request) {
    _EDEBUGTHIS("write, state: %d", (int)state_);
    NextFilter::write(write_request);
}

void SocksFilter::on_write(const CodeError* err, WriteRequest* write_request) {
    _EDEBUGTHIS("on_write, err: %d, state: %d", err ? err->code() : 0, (int)state_);
    if (state_ == State::terminal) {
        NextFilter::on_write(err, write_request);
        return;
    }

    write_request->event(handle, err, write_request);
    write_request->release();
    handle->release();
}

void SocksFilter::on_read(string& buf, const CodeError* err) {
    _EDEBUG("on_read, %lu bytes, state %d", buf.length(), (int)state_);
    if (state_ == State::terminal) {
        NextFilter::on_read(buf, err);
        return;
    }

    _EDUMP(buf, (int)buf.length(), 100);

    // pointer to current buffer
    const char* buffer_ptr = buf.data();
    // start parsing from the beginning pointer
    const char* p = buffer_ptr;
    // to the end pointer
    const char* pe = buffer_ptr + buf.size();
    
    const char* eof = pe;

    // select reply parser by our state
    switch (state_) {
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
            // need more input
            break;
        case State::error:
            _EDEBUGTHIS("error state, wont parse");
            return;
        default:
            _EDEBUGTHIS("bad state, len: %d", int(p - buffer_ptr));
            do_error();
            return;
    }

    state_ = State::parsing;

// generated parser logic
#define MACHINE_EXEC
#include "SocksParser.cc"

    if (state_ == State::error) {
        _EDEBUGTHIS("parser exiting in error state on pos: %d", int(p - buffer_ptr));
    } else if (state_ != State::parsing) {
        _EDEBUGTHIS("parser finished");
    }
}

void SocksFilter::on_shutdown(const CodeError* err, ShutdownRequest* req) {
    _EDEBUGTHIS("on_shutdown, err: %d", err ? err->code() : 0);
    NextFilter::on_shutdown(err, req);
}

void SocksFilter::on_eof() {
    _EDEBUGTHIS("on_eof, state: %d", (int)state_);
    if(state_ == State::terminal) {
        NextFilter::on_eof();
        return;
    }

    if (state_ == State::parsing || state_ == State::handshake_reply || state_ == State::auth_reply || state_ == State::connect_reply) {
        do_error();
        return;
    }
}

void SocksFilter::on_reinit() {
    _EDEBUGTHIS("on_reinit, state: %d, connecting: %d", (int)state_, (int)handle->connecting());
    if(state_ == State::terminal) {
        NextFilter::on_reinit();
        return;
    }
    
    if (state_ == State::handshake_reply || state_ == State::auth_reply || state_ == State::connect_reply) {
        state(State::initial);
        NextFilter::on_connect(CodeError(ERRNO_ECANCELED), connect_request_);
        return;
    }

    if(state_ == State::connecting_proxy) {
        state(State::initial);
    }
}

void SocksFilter::do_handshake() {
    _EDEBUGTHIS("do_handshake");
    SocksHandshakeRequest* socks_handshake_request = new SocksHandshakeRequest(
        [=](Stream*, const CodeError* err, WriteRequest*) {
            _EDEBUGTHIS("handshake callback");
            if (err || this->state_ == State::canceled) {
                do_error();
                return;
            }

            state(State::handshake_reply);
        },
        socks_);

    state(State::handshake_write);
    handle->attach(socks_handshake_request);
    handle->retain();
    socks_handshake_request->retain();
    NextFilter::write(socks_handshake_request);
}

void SocksFilter::do_auth() {
    _EDEBUGTHIS("do_auth");
    SocksAuthRequest* socks_auth_request = new SocksAuthRequest(
        [=](Stream*, const CodeError* err, WriteRequest*) {
            _EDEBUGTHIS("auth callback");
            if (err || this->state_ == State::canceled) {
                do_error();
                return;
            }

            state(State::auth_reply);
        },
        socks_);

    state(State::auth_write);
    handle->attach(socks_auth_request);
    handle->retain();
    socks_auth_request->retain();
    NextFilter::write(socks_auth_request);
}

void SocksFilter::do_resolve() {
    _EDEBUGTHIS("do_resolve_host");
    TCP* tcp         = dyn_cast<TCP*>(handle);
    tcp->resolver()->resolve(host_, panda::to_string(port_), nullptr, [=](SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
        _EDEBUGTHIS("resolved err:%d", err ? err->code() : 0);
        if (err || this->state_ == State::canceled) {
            do_error();
            return;
        }

        sa_ = address->head->ai_addr;
        do_handshake();
    });

    state(State::resolving_host);
}

void SocksFilter::do_connect() {
    _EDEBUGTHIS("do_connect");
    SocksCommandConnectRequest* socks_command_connect_request;
    if (sa_) {
        socks_command_connect_request = new SocksCommandConnectRequest(
            [=](Stream*, const CodeError* err, WriteRequest*) {
                _EDEBUGTHIS("command connect callback %d", err ? err->code() : 0);
                if (err || this->state_ == State::canceled) {
                    do_error();
                    return;
                }

                state(State::connect_reply);
            },
            sa_);
    } else {
        socks_command_connect_request = new SocksCommandConnectRequest(
            [=](Stream*, const CodeError* err, WriteRequest*) {
                _EDEBUGTHIS("command connect callback %d", err ? err->code() : 0);
                if (err || this->state_ == State::canceled) {
                    _EDEBUGTHIS("leaving");
                    do_error();
                    return;
                }

                state(State::connect_reply);
            },
            host_, port_);
    }
    state(State::connect_write);
    handle->attach(socks_command_connect_request);
    handle->retain();
    socks_command_connect_request->retain();
    NextFilter::write(socks_command_connect_request);
}

void SocksFilter::do_connected() {
    _EDEBUGTHIS("do_connected");
    state(State::terminal);
    restore_read_start(); // stop reading if handle don't want to
    NextFilter::on_connect(nullptr, connect_request_);
}

void SocksFilter::do_error(const CodeError* err) {
    _EDEBUGTHIS("do_error");
    if(state_ != State::error) {
        init_parser();
        if(err && err->code() == ERRNO_ECANCELED) {
            state(State::initial);
        } else {
            state(State::error);
        }
        restore_read_start();
        NextFilter::on_connect(err, connect_request_);
    }
}

}}} // namespace panda::event::socks
