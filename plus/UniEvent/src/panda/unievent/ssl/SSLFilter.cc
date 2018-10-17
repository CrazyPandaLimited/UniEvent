#include "SSLFilter.h"

#include <vector>

#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/ssl.h>

#include <panda/unievent/Prepare.h>
#include "../Stream.h"
#include "SSLBio.h"
#include "../Debug.h"


#define PROFILE_STR profile == Profile::CLIENT ? "client" : "server"

#define _ESSL(fmt, ...) do { if(EVENT_LIB_DEBUG >= 1) fprintf(stderr, "%s:%d:%s(): [%s] {%p} " fmt "\n", __FILE__, __LINE__, __func__, profile == Profile::CLIENT ? "client" : "server", this->handle, ##__VA_ARGS__); } while (0)

namespace panda { namespace unievent { namespace ssl {

const char* SSLFilter::TYPE = "SSL";
bool        SSLFilter::openSSL_inited = SSLFilter::init_openSSL_lib();

class SSLWriteRequest : public WriteRequest {
public:
    WriteRequest* src;
    ~SSLWriteRequest(){ _EDTOR(); }
    SSLWriteRequest (WriteRequest* src = nullptr) : WriteRequest(), src(src) { _ECTOR(); }
};

SSLFilter::~SSLFilter () {
    _EDTOR();
    SSL_free(ssl);
}

SSLFilter::SSLFilter(SSLFilter* parent_filter, Stream* stream, SSL_CTX* context)
        : StreamFilter(stream, TYPE), connect_request(nullptr), parent_filter(parent_filter), state(State::initial), profile(Profile::UNKNOWN) {
    _ECTOR();
    init(context);
}

SSLFilter::SSLFilter(Stream* stream, SSL_CTX* context)
        : StreamFilter(stream, TYPE), connect_request(nullptr), parent_filter(nullptr), state(State::initial), profile(Profile::UNKNOWN) {
    _ECTOR();
    init(context);
}

SSLFilter::SSLFilter(Stream* stream, const SSL_METHOD* method)
        : StreamFilter(stream, TYPE), connect_request(nullptr), parent_filter(nullptr), state(State::initial), profile(Profile::UNKNOWN) {
    _ECTOR();
    if (!method) {
        method = SSLv23_client_method();
        profile = Profile::CLIENT;
    }
    SSL_CTX* context = SSL_CTX_new(method);
    if (!context)
        throw SSLError(SSL_ERROR_SSL);
    init(context);
    SSL_CTX_free(context); // it is refcounted, ssl keeps it if neded
}

void SSLFilter::init (SSL_CTX* context) {
    ssl = SSL_new(context);
//     SSL_CTX_set_msg_callback(context, &msg);
//     SSL_CTX_set_info_callback(context, sslInfoCallback);  // for debug
    if (!ssl) throw SSLError(SSL_ERROR_SSL);
    read_bio = BIO_new(SSLBio::method());
    write_bio = BIO_new(SSLBio::method());
    if (!read_bio || !write_bio) throw SSLError(SSL_ERROR_SSL);
    SSLBio::set_handle(read_bio, handle);
    SSLBio::set_handle(write_bio, handle);
    SSL_set_bio(ssl, read_bio, write_bio);
}

void SSLFilter::reset () {
    _ESSL("reset, state: %d, connecting: %d", (int)state, handle->connecting());
    if (state == State::initial) {
        return;
    }

    ERR_clear_error();
    state = State::initial;
    // hard reset
    SSL* oldssl = ssl;
    init(SSL_get_SSL_CTX(ssl));
    SSL_free(oldssl);
    //// soft reset - openssl docs say it should work, but IT DOES NOT WORK!
    //if (!SSL_clear(ssl)) throw SSLError(SSL_ERROR_SSL);
}

void SSLFilter::on_connect (const CodeError* err, ConnectRequest* req) {
    _ESSL("on_connect, ERR=%d WHAT=%s", err ? err->code() : 0, err ? err->what() : "");
    //if (state == State::terminal) {
        // we need this for ssl filters chaining
        //NextFilter::on_connect(err, req);
        //return;
    //}

    reset();

    if (err) {
        NextFilter::on_connect(err, req);
        return;
    }

    auto read_err = temp_read_start();
    if (read_err) { 
        NextFilter::on_connect(read_err, req); 
        return;
    }

    connect_request = req;
    start_ssl_connection(Profile::CLIENT);
}

void SSLFilter::on_connection (Stream* stream, const CodeError* err) {
    _ESSL("on_connection, stream: %p, err: %d", stream, err ? err->code() : 0);
    if (err) {
        NextFilter::on_connection(handle, err);
        return;
    }

    stream->async_lock();
    SSLFilter* filter = new SSLFilter(this, stream, SSL_get_SSL_CTX(ssl));
    stream->add_filter(filter);
    auto uverr = filter->temp_read_start();
    if (uverr) {
        NextFilter::on_connection(handle, uverr);
        return;
    }

    filter->start_ssl_connection(Profile::SERVER);
}

void SSLFilter::start_ssl_connection (Profile profile) {
    _ESSL();

    this->profile = profile;
    if (profile == Profile::CLIENT) {
        SSL_set_connect_state(ssl);
    } else {
        SSL_set_accept_state(ssl);
    }

    negotiate();
}

int SSLFilter::negotiate () {
    assert((!SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl)) && handle->connecting());
    
    state = State::negotiating;
    
    int ssl_state = SSL_do_handshake(ssl);

    _ESSL("ssl_state=%d, renego pending %d", ssl_state, SSL_renegotiate_pending(ssl));

    string write_buf = SSLBio::steal_buf(write_bio);
    if (write_buf) {
        _ESSL("writing %d bytes", (int)write_buf.length());
        SSLWriteRequest* req = new SSLWriteRequest();
        req->retain();
        handle->attach(req);
        req->bufs.push_back(write_buf);
        NextFilter::write(req);
    }

    if (ssl_state <= 0) {
        int code = SSL_get_error(ssl, ssl_state);
        _ESSL("code=%d", code);
        if (code != SSL_ERROR_WANT_READ && code != SSL_ERROR_WANT_WRITE) {
            SSLBio::steal_buf(read_bio); // avoid clearing by BIO
            negotiation_finished(SSLError(code));
            return 0;
        }
    }

    if (SSL_is_init_finished(ssl)) {
        negotiation_finished();
        return BIO_ctrl(read_bio, BIO_CTRL_PENDING, 0, nullptr);
    }
    return 0;
}

//// renegotiate DOES NOT WORK - SSL_is_init_finshed returns true, SSL_renegotiate_pending becomes false after first SSL_do_handshake
//void SSLFilter::renegotiate () {
//    printf("SSLFilter::renegotiate\n");
//    if (!SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl) || !handle->connected()) throw StreamError(ERRNO_EINVAL);
//    SSL_set_verify(ssl, SSL_VERIFY_PEER, nullptr);
//    printf("SSL_get_secure_renegotiation_support=%li\n", SSL_get_secure_renegotiation_support(ssl));
//    int ssl_state = SSL_renegotiate(ssl);
//    printf("SSLFilter::renegotiate: renego pending %d\n", SSL_renegotiate_pending(ssl));
//    if (ssl_state <= 0) {
//        int code = SSL_get_error(ssl, ssl_state);
//        throw SSLError(code);
//    }
//    handle->connected(false);
//    handle->connecting(true);
//    negotiate();
//}

void SSLFilter::negotiation_finished (const CodeError* err) {
    _ESSL("connecting: %d err=%s", (int)handle->connecting(), err ? err->what() : "");

    if(state == State::terminal || state == State::error) {
        return;
    }

    if(err) {
        state = State::error;
    } else {
        state = State::terminal;
    }

    restore_read_start(); // stop reading if handle don't want to
    if (profile == Profile::CLIENT) {
        NextFilter::on_connect(err, connect_request);
        connect_request = nullptr;
    }
    else {
        parent_filter->NextFilter::on_connection(handle, err);
        handle->async_unlock();
    }
}

void SSLFilter::on_read (string& encbuf, const CodeError* err) {
    _ESSL("got %lu bytes, state: %d", encbuf.length(), (int)state);
    if(state == State::error) {
        NextFilter::on_read(encbuf, SSLError(SSL_ERROR_SSL));
        return;
    }

    if (!handle->connecting() && !handle->connected()) {
        _ESSL("on_read strange state: neither connecting nor connected, possibly server is not configured");
    }

    bool connecting = !SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl);
    if (err) {
        if (!handle->connecting()) { 
            // if not connecting then it's ongoing packets for already failed handshake, just ignore them
        }
        else if (connecting) { 
            negotiation_finished(err);
        }
        else {
            NextFilter::on_read(encbuf, err);
        }
        return;
    }

    SSLBio::set_buf(read_bio, encbuf);

    int pending = encbuf.length();
    if (connecting && !(pending = negotiate())) { // no more data to read, handshake not completed
        SSLBio::steal_buf(read_bio);
        return;
    }

    int ret;
    string decbuf = handle->buf_alloc(pending);
    while ((ret = read_ssl_buffer(decbuf, pending)) > 0) {
        decbuf.length(ret);
        NextFilter::on_read(decbuf, err);
    }

    int ssl_code = SSL_get_error(ssl, ret);
    _ESSL("errno=%d, err=%d", ssl_code, ERR_GET_LIB(ERR_peek_last_error()));

    if (ssl_code == SSL_ERROR_ZERO_RETURN || ssl_code == SSL_ERROR_WANT_READ) {
        return;
    }

    if (ssl_code == SSL_ERROR_WANT_WRITE) { // renegotiation
        string wbuf = SSLBio::steal_buf(write_bio);
        _ESSL("write %lu", wbuf.length());
        SSLWriteRequest* req = new SSLWriteRequest();
        req->retain();
        handle->attach(req);
        req->bufs.push_back(wbuf);
        NextFilter::write(req);
    } else {
        string s;
        NextFilter::on_read(s, SSLError(ssl_code));
    }
}

int SSLFilter::read_ssl_buffer(string& decbuf, int pending) {
    int ret = SSL_read(ssl, decbuf.buf(), pending);
    _ESSL("sslret=%d pending=%d", ret, pending);
    return ret;
}

void SSLFilter::write (WriteRequest* req) {
    if(state != State::terminal) {
        NextFilter::on_write(SSLError(SSL_ERROR_SSL), req);
        return;
    }

    SSLWriteRequest* sslreq = new SSLWriteRequest(req);
    sslreq->retain();
    handle->attach(sslreq);
    
    _ESSL("request: %p, sslrequest: %p", req, sslreq);

    auto bufcnt = req->bufs.size();
    sslreq->bufs.reserve(bufcnt);
    for (size_t i = 0; i < bufcnt; i++) {
        if (req->bufs[i].length() == 0) {
            continue;
        }
        int res = SSL_write(ssl, req->bufs[i].data(), req->bufs[i].length());
        if (res <= 0) {
            // TODO: handle renegotiation status
            _ESSL("ssl failed");
            NextFilter::on_write(SSLError(SSL_ERROR_SSL), req);
            sslreq->release();
            return;
        }
        string buf = SSLBio::steal_buf(write_bio);
        sslreq->bufs.push_back(buf);
    }

    NextFilter::write(sslreq);
}

void SSLFilter::on_write (const CodeError* err, WriteRequest* req) {
    _ESSL("on_write state: %d", (int)state);

    SSLWriteRequest* sslreq = static_cast<SSLWriteRequest*>(req);
    _ESSL("request=%p regular=%d ERR=%s", req, sslreq->src ? 1 : 0, err ? err->what() : "");
    if (sslreq->src) { 
        // regular on_write
        NextFilter::on_write(err, sslreq->src);
    } else { 
        // negotiation on_write
        if (err) negotiation_finished(err);
    }

    sslreq->release();
}

void SSLFilter::on_reinit () {
    _ESSL("on_reinit, state: %d, connecting: %d", (int)state, handle->connecting());
    if (state == State::negotiating) {
        negotiation_finished(CodeError(ERRNO_ECANCELED));
    } else if (state == State::terminal) {
        NextFilter::on_reinit();
    }
}

void SSLFilter::on_eof() {
    _ESSL("on_eof, state: %d", (int)state);
    if(state == State::terminal) {
        NextFilter::on_eof();
    }
    else if(state == State::negotiating) {
        negotiation_finished(CodeError(ERRNO_EOF));
    }
}

void SSLFilter::on_shutdown(const CodeError* err, ShutdownRequest* shutdown_request) {
    _ESSL("on_shutdown, state: %d", (int)state);
    NextFilter::on_shutdown(err, shutdown_request);
}

bool SSLFilter::is_secure () { return true; }

bool SSLFilter::init_openSSL_lib () {
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    return true;
}

}}}
