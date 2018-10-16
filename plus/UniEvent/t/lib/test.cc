#include "test.h"
#include <memory>
#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/engine.h>

bool TEST_SSL;
bool TEST_SOCKS;
bool TEST_BUF;

SSL_CTX* get_ssl_ctx() {
    static SSL_CTX* ctx = nullptr;
    if (ctx) {
        return ctx;
    }
    ctx = SSL_CTX_new(SSLv23_server_method());
    SSL_CTX_use_certificate_file(ctx, "./cert.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "./key.pem", SSL_FILETYPE_PEM);
    SSL_CTX_check_private_key(ctx);
    return ctx;
}

bool parse_bool_parameter(const char* value) { return value && (strcmp(value, "1") == 0 || strcmp(value, "true")); }

static iptr<Socks> getenv_proxy () {
    // PANDA_EVENT_PROXY="socks5://[user:password@]proxyhost[:port]"
    const char* env_proxy = getenv("UNIEVENT_PROXY");
    // PANDA_EVENT_PROXY_RESOLVE="1"
    const char* env_proxy_resolve = getenv("UNIEVENT_PROXY_RESOLVE");

    static iptr<Socks> socks;
    if (env_proxy) {
        socks = new Socks(string(env_proxy), parse_bool_parameter(env_proxy_resolve));
    }
    return socks;
}

TCPSP make_basic_server (uint16_t port, Loop* loop) {
    TCPSP server = new TCP(loop);
    server->bind("localhost", panda::to_string(port));
    server->listen(1);
    return server;
}

TCPSP make_socks_server (uint16_t port, iptr<Loop> loop) {
    TCPSP server = new TCP(loop);
    server->bind("localhost", panda::to_string(port));
    server->listen(1);
    server->connection_event.add([port](Stream* server, iptr<Stream> stream, const CodeError* err) {
        assert(!err);
        std::shared_ptr<int> state = std::make_shared<int>(0);
        TCPSP client = new TCP(server->loop());
        read(stream, [client, state](iptr<Stream> stream, const string& buf, const CodeError* err) {
            _EDUMP(buf, (int)buf.length(), 100);
            assert(!err);
            switch (*state) {
                case 0: {
                    stream->write("\x05\x00");
                    *state = 1;
                    break;
                }
                case 1: {
                    string request_type = buf.substr(0, 4);
                    if (request_type == string("\x05\x01\x00\x03")) {
                        int host_length = buf[4];
                        string host = buf.substr(5, host_length);
                        uint16_t port = ntohs(*(uint16_t*)buf.substr(5 + host_length).data());
                        client->connect("localhost", panda::to_string(port));
                    } else {
                        throw std::runtime_error("bad request");
                    }

                    read(client, [stream](Stream*, string& buf, const CodeError* err){
                        assert(!err);
                        // read from remote server
                        stream->write(buf);
                    });
                    client->eof_event.add_back([stream](Stream* client){
                        stream->shutdown();
                        client->eof_event.remove_all();
                    });

                    stream->write("\x05\x00\x00\x01\xFF\xFF\xFF\xFF\xFF\xFF");
                    *state = 2;
                    break;
                }
                case 2: {
                    // write to remote server
                    client->write(buf);
                    break;
                }
            }
        });
    });

    return server;
}

TCPSP make_server (uint16_t port, Loop* loop) {
    TCPSP server = new TCP(loop);
    server->bind("localhost", panda::to_string(port));
    if (TEST_SSL) {
        server->use_ssl(get_ssl_ctx());
    }
    server->listen(1);
    return server;
}

TCPSP make_client (Loop* loop, bool cached_resolver) {
    TCPSP client = new TCP(loop, cached_resolver);
    if (TEST_SSL) {
        client->use_ssl();
    }

    if (TEST_SOCKS) {
        client->use_socks(getenv_proxy());
    }

    if (TEST_BUF) {
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
