#include "test.h"
#include <memory>
#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/engine.h>

Variation variation;

SSL_CTX* get_ssl_ctx() {
    static SSL_CTX* ctx = nullptr;
    if (ctx) {
        return ctx;
    }
    ctx = SSL_CTX_new(SSLv23_server_method());
    SSL_CTX_use_certificate_file(ctx, "t/cert/cert.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "t/cert/key.pem", SSL_FILETYPE_PEM);
    SSL_CTX_check_private_key(ctx);
    return ctx;
}

TCPSP make_basic_server (Loop* loop, const SockAddr& sa) {
    TCPSP server = new TCP(loop);
    server->bind(sa);
    server->listen(100);
    return server;
}

TCPSP make_server (Loop* loop, const SockAddr& sa) {
    TCPSP server = new TCP(loop);
    server->bind(sa);
    if (variation.ssl) server->use_ssl(get_ssl_ctx());
    server->listen(10000);
    return server;
}

TCPSP make_client (Loop* loop, bool cached_resolver) {
    TCPSP client = new TCP(loop, cached_resolver);

    if (variation.ssl) client->use_ssl();

    if (variation.buf) {
        client->set_recv_buffer_size(1);
        client->set_send_buffer_size(1);
    }

    return client;
}

TimerSP read (StreamSP stream, Stream::read_fn callback, uint64_t timeout) {
    TimerSP timer = new Timer(stream->loop());
    _EDEBUG("read timer %p", static_cast<Handle*>(timer.get()));
    timer->timer_event.add([stream, callback](Timer*) {
        string empty;
        callback(stream, empty, CodeError(ERRNO_ETIMEDOUT));
    });
    timer->once(timeout);

    stream->read_start([stream, timer, callback](Stream*, string& buf, const CodeError* err) mutable {
        timer->reset();
        timer->timer_event.remove_all();
        callback(stream, buf, err);
    });
    stream->eof_event.add_back([](Stream* stream){
        _EDEBUG("eof for %p %d readlistn=%d", static_cast<Handle*>(stream), stream->refcnt(), stream->read_event.has_listeners());
        stream->read_event.remove_all();
        _EDEBUG("eof for %p %d", static_cast<Handle*>(stream), stream->refcnt());

    });
    stream->shutdown_event.add([](Stream* stream, const CodeError*, ShutdownRequest*){
        _EDEBUG("shutdown for %p %d readlistn=%d", static_cast<Handle*>(stream), stream->refcnt(), stream->read_event.has_listeners());
        stream->read_event.remove_all();
        _EDEBUG("shutdown for %p %d", static_cast<Handle*>(stream), stream->refcnt());

    });

    return timer;
}
