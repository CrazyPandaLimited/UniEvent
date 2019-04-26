#include "SslFilter.h"
#include "SslBio.h"
#include "../Debug.h"
#include "../Stream.h"
//#include <vector>
#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/ssl.h>

#define PROFILE_STR profile == Profile::CLIENT ? "client" : "server"

#define _ESSL(fmt, ...) do { if(EVENT_LIB_DEBUG >= 0) fprintf(stderr, "%s:%d:%s(): [%s] {%p} " fmt "\n", __FILE__, __LINE__, __func__, profile == Profile::CLIENT ? "client" : (profile == Profile::SERVER ? "server" : "no profile"), this->handle, ##__VA_ARGS__); } while (0)

namespace panda { namespace unievent { namespace ssl {

const void* SslFilter::TYPE = &typeid(SslFilter);

static bool init_openssl_lib () {
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    return true;
}
static const bool _init = init_openssl_lib();

static SSL_CTX* ssl_ctx_from_method (const SSL_METHOD* method) {
    if (!method) method = SSLv23_client_method();
    auto context = SSL_CTX_new(method);
    if (!context) throw SSLError(SSL_ERROR_SSL);
    return context;
}

SslFilter::SslFilter (Stream* stream, SSL_CTX* context, const SslFilterSP& server_filter)
        : StreamFilter(stream, TYPE, PRIORITY), /*connect_request(nullptr),*/ state(State::initial), profile(Profile::UNKNOWN), server_filter(server_filter)
{
    _ECTOR();
    if (stream->listening() && !SSL_CTX_check_private_key(context)) throw Error("SSL certificate&key needed to listen()");

    ssl = SSL_new(context);
    if (!ssl) throw SSLError(SSL_ERROR_SSL);

    read_bio = BIO_new(SslBio::method());
    if (!read_bio) throw SSLError(SSL_ERROR_SSL);

    write_bio = BIO_new(SslBio::method());
    if (!write_bio) throw SSLError(SSL_ERROR_SSL);

    SslBio::set_handle(read_bio, handle);
    SslBio::set_handle(write_bio, handle);
    SSL_set_bio(ssl, read_bio, write_bio);
}

SslFilter::SslFilter (Stream* stream, const SSL_METHOD* method) : SslFilter(stream, ssl_ctx_from_method(method)) {
    SSL_CTX_free(SSL_get_SSL_CTX(ssl)); // it is refcounted, release ctx created from ssl_ctx_from_method
}

SslFilter::~SslFilter () {
    _EDTOR();
    SSL_free(ssl);
}

void SslFilter::listen () {
    if (!SSL_check_private_key(ssl)) throw Error("SSL certificate&key needed to listen()");
    NextFilter::listen();
}

//struct SSLWriteRequest : WriteRequest {
//    WriteRequest* src;
//    ~SSLWriteRequest(){ _EDTOR(); }
//    SSLWriteRequest (WriteRequest* src = nullptr) : WriteRequest(), src(src) { _ECTOR(); }
//};
//
//void SslFilter::reset () {
//    _ESSL("reset, state: %d, connecting: %d", (int)state, handle->connecting());
//    if (state == State::initial) return;
//
//    ERR_clear_error();
//    state = State::initial;
//    // hard reset
//    SSL* oldssl = ssl;
//    init(SSL_get_SSL_CTX(ssl));
//    SSL_free(oldssl);
//    //// soft reset - openssl docs say it should work, but IT DOES NOT WORK!
//    //if (!SSL_clear(ssl)) throw SSLError(SSL_ERROR_SSL);
//}

void SslFilter::handle_connect (const CodeError& err, const ConnectRequestSP& req) {
    _ESSL("ERR=%s", err.what());

return NextFilter::handle_connect(err, req);
    //if (state == State::terminal) {
        // we need this for ssl filters chaining
        //NextFilter::on_connect(err, req);
        //return;
    //}

    //reset();

    if (err) return NextFilter::handle_connect(err, req);

    auto read_err = read_start();
    if (read_err) return NextFilter::handle_connect(read_err, req);

    connect_request = req;
    start_ssl_connection(Profile::CLIENT);
}

void SslFilter::handle_connection (const StreamSP& client, const CodeError& err) {
    _ESSL("client: %p, err: %s", client.get(), err.what());
return NextFilter::handle_connection(client, err);
    if (err) return NextFilter::handle_connection(client, err);

    SslFilter* filter = new SslFilter(client, SSL_get_SSL_CTX(ssl), this);
    client->add_filter(filter);

    auto read_err = filter->read_start();
    if (read_err) return NextFilter::handle_connection(client, read_err);

    filter->start_ssl_connection(Profile::SERVER);
}

void SslFilter::start_ssl_connection (Profile profile) {
    _ESSL();

    this->profile = profile;
    if (profile == Profile::CLIENT) SSL_set_connect_state(ssl);
    else                            SSL_set_accept_state(ssl);

    //negotiate();
}

//int SslFilter::negotiate () {
//    bool renegotiate = SSL_renegotiate_pending(ssl);
//    assert((!SSL_is_init_finished(ssl) && handle->connecting()) || renegotiate);
//
//    state = State::negotiating;
//
//    int ssl_state = SSL_do_handshake(ssl);
//
//    _ESSL("ssl_state=%d, renego pending %d", ssl_state, SSL_renegotiate_pending(ssl));
//
//    string write_buf = SslBio::steal_buf(write_bio);
//    if (write_buf) {
//        _ESSL("writing %d bytes", (int)write_buf.length());
//        SSLWriteRequest* req = new SSLWriteRequest();
//        req->retain();
//        handle->attach(req);
//        req->bufs.push_back(write_buf);
//        NextFilter::write(req);
//    }
//
//    if (ssl_state <= 0) {
//        int code = SSL_get_error(ssl, ssl_state);
//        _ESSL("code=%d", code);
//        if (code != SSL_ERROR_WANT_READ && code != SSL_ERROR_WANT_WRITE) {
//            SslBio::steal_buf(read_bio); // avoid clearing by BIO
//            negotiation_finished(SSLError(code));
//            return 0;
//        }
//    }
//
//    if (SSL_is_init_finished(ssl)) {
//        negotiation_finished();
//        return BIO_ctrl(read_bio, BIO_CTRL_PENDING, 0, nullptr);
//    }
//    return 0;
//}
//
////// renegotiate DOES NOT WORK - SSL_is_init_finshed returns true, SSL_renegotiate_pending becomes false after first SSL_do_handshake
////void SslFilter::renegotiate () {
////    printf("SslFilter::renegotiate\n");
////    if (!SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl) || !handle->connected()) throw StreamError(ERRNO_EINVAL);
////    SSL_set_verify(ssl, SSL_VERIFY_PEER, nullptr);
////    printf("SSL_get_secure_renegotiation_support=%li\n", SSL_get_secure_renegotiation_support(ssl));
////    int ssl_state = SSL_renegotiate(ssl);
////    printf("SslFilter::renegotiate: renego pending %d\n", SSL_renegotiate_pending(ssl));
////    if (ssl_state <= 0) {
////        int code = SSL_get_error(ssl, ssl_state);
////        throw SSLError(code);
////    }
////    handle->connected(false);
////    handle->connecting(true);
////    negotiate();
////}
//
//void SslFilter::negotiation_finished (const CodeError* err) {
//    _ESSL("connecting: %d err=%s", (int)handle->connecting(), err ? err->what() : "");
//
//    if(state == State::terminal || state == State::error) {
//        return;
//    }
//
//    if (handle->connected()) { // prevent double callback call after renegotiate
//        state = State::terminal;
//        return;
//    }
//
//    if(err) {
//        state = State::error;
//    } else {
//        state = State::terminal;
//    }
//
//    restore_read_start(); // stop reading if handle don't want to
//    if (profile == Profile::CLIENT) {
//        NextFilter::on_connect(err, connect_request);
//        connect_request = nullptr;
//    }
//    else {
//        if (auto parent = parent_filter.lock()) {
//            parent->NextFilter::on_connection(handle, err);
//        }
//        handle->async_unlock();
//        handle->release();
//    }
//}
//
//void SslFilter::on_read (string& encbuf, const CodeError* err) {
//    _ESSL("got %lu bytes, state: %d", encbuf.length(), (int)state);
//    if(state == State::error) {
//        NextFilter::on_read(encbuf, SSLError(SSL_ERROR_SSL));
//        return;
//    }
//
//    if (!handle->connecting() && !handle->connected()) {
//        _ESSL("on_read strange state: neither connecting nor connected, possibly server is not configured");
//    }
//
//    _EDEBUG("ssl_init_finished %d, renegotiate %d", SSL_is_init_finished(ssl), SSL_renegotiate_pending(ssl));
//    bool connecting = !SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl);
//    if (err) {
//        if (!handle->connecting()) {
//            // if not connecting then it's ongoing packets for already failed handshake, just ignore them
//        }
//        else if (connecting) {
//            negotiation_finished(err);
//        }
//        else {
//            NextFilter::on_read(encbuf, err);
//        }
//        return;
//    }
//
//    SslBio::set_buf(read_bio, encbuf);
//
//    _EDEBUG("connecting %d, err %s", connecting, err ? err->what() : "");
//
//    int pending = encbuf.length();
//    if (connecting && !(pending = negotiate())) { // no more data to read, handshake not completed
//        SslBio::steal_buf(read_bio);
//        return;
//    }
//
//    int ret;
//    string decbuf = handle->buf_alloc(pending);
//    while ((ret = read_ssl_buffer(decbuf, pending)) > 0) {
//        decbuf.length(ret);
//        NextFilter::on_read(decbuf, err);
//    }
//
//    int ssl_code = SSL_get_error(ssl, ret);
//    _ESSL("errno=%d, err=%d", ssl_code, ERR_GET_LIB(ERR_peek_last_error()));
//
//    if (ssl_code == SSL_ERROR_ZERO_RETURN || ssl_code == SSL_ERROR_WANT_READ) {
//        return;
//    }
//
//    if (ssl_code == SSL_ERROR_WANT_WRITE) { // renegotiation
//        string wbuf = SslBio::steal_buf(write_bio);
//        _ESSL("write %lu", wbuf.length());
//        SSLWriteRequest* req = new SSLWriteRequest();
//        req->retain();
//        handle->attach(req);
//        req->bufs.push_back(wbuf);
//        NextFilter::write(req);
//    } else {
//        string s;
//        NextFilter::on_read(s, SSLError(ssl_code));
//    }
//}
//
//int SslFilter::read_ssl_buffer(string& decbuf, int pending) {
//    int ret = SSL_read(ssl, decbuf.buf(), pending);
//    _ESSL("sslret=%d pending=%d", ret, pending);
//    return ret;
//}
//
//void SslFilter::write (WriteRequest* req) {
//    if(state != State::terminal) {
//        NextFilter::on_write(SSLError(SSL_ERROR_SSL), req);
//        return;
//    }
//
//    SSLWriteRequest* sslreq = new SSLWriteRequest(req);
//    sslreq->retain();
//    handle->attach(sslreq);
//
//    _ESSL("request: %p, sslrequest: %p", req, sslreq);
//
//    auto bufcnt = req->bufs.size();
//    sslreq->bufs.reserve(bufcnt);
//    for (size_t i = 0; i < bufcnt; i++) {
//        if (req->bufs[i].length() == 0) {
//            continue;
//        }
//        int res = SSL_write(ssl, req->bufs[i].data(), req->bufs[i].length());
//        if (res <= 0) {
//            // TODO: handle renegotiation status
//            _ESSL("ssl failed");
//            NextFilter::on_write(SSLError(SSL_ERROR_SSL), req);
//            sslreq->release();
//            return;
//        }
//        string buf = SslBio::steal_buf(write_bio);
//        sslreq->bufs.push_back(buf);
//    }
//
//    NextFilter::write(sslreq);
//}
//
//void SslFilter::on_write (const CodeError* err, WriteRequest* req) {
//    _ESSL("on_write state: %d", (int)state);
//
//    SSLWriteRequest* sslreq = static_cast<SSLWriteRequest*>(req);
//    _ESSL("request=%p regular=%d ERR=%s", req, sslreq->src ? 1 : 0, err ? err->what() : "");
//    if (sslreq->src) {
//        // regular on_write
//        NextFilter::on_write(err, sslreq->src);
//    } else {
//        // negotiation on_write
//        if (err) negotiation_finished(err);
//    }
//
//    sslreq->release();
//}
//
//void SslFilter::on_reinit () {
//    _ESSL("on_reinit, state: %d, connecting: %d", (int)state, handle->connecting());
//    if (state == State::negotiating) {
//        negotiation_finished(CodeError(ERRNO_ECANCELED));
//    } else if (state == State::terminal) {
//        NextFilter::on_reinit();
//    }
//}
//
//void SslFilter::on_eof() {
//    _ESSL("on_eof, state: %d", (int)state);
//    if(state == State::terminal) {
//        NextFilter::on_eof();
//    }
//    else if(state == State::negotiating) {
//        negotiation_finished(CodeError(ERRNO_EOF));
//    }
//}
//
//void SslFilter::on_shutdown(const CodeError* err, ShutdownRequest* shutdown_request) {
//    _ESSL("on_shutdown, state: %d", (int)state);
//    NextFilter::on_shutdown(err, shutdown_request);
//}

}}}
