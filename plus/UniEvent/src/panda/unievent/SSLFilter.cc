#include "SSLFilter.h"
#include <vector>

#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/ssl.h>

#include "Stream.h"
#include "SSLBio.h"
#include "Debug.h"


#define PROFILE_STR profile == Profile::CLIENT ? "client" : "server"

using namespace panda::unievent;

const char* SSLFilter::TYPE = "SSL";
bool        SSLFilter::openSSL_inited = SSLFilter::init_openSSL_lib();

class SSLWriteRequest : public WriteRequest {
public:
    WriteRequest* src;
    SSLWriteRequest (WriteRequest* src) : WriteRequest(), src(src) {}
};

SSLFilter::SSLFilter (Stream* h, SSL_CTX* context) : StreamFilter(h), connect_request(nullptr), started(false) {
    _type = TYPE;
    init(context);
}

SSLFilter::SSLFilter (Stream* h, const SSL_METHOD* method) : StreamFilter(h), connect_request(nullptr), started(false) {
    _type = TYPE;
    if (!method) method = SSLv23_client_method();
    SSL_CTX* context = SSL_CTX_new(method);
    if (!context) throw SSLError(SSL_ERROR_SSL);
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
    if (!started) return;
    ERR_clear_error();
    started = false;
    // hard reset
    SSL* oldssl = ssl;
    init(SSL_get_SSL_CTX(ssl));
    SSL_free(oldssl);
    //// soft reset - openssl docs say it should work, but IT DOES NOT WORK!
    //if (!SSL_clear(ssl)) throw SSLError(SSL_ERROR_SSL);
}

SSLFilter::~SSLFilter () {
    SSL_free(ssl);
}

void SSLFilter::on_connect (const CodeError* err, ConnectRequest* req) {
    if (err) _EDEBUG("ERR=%s", err->what());

    if (started) reset();
    if (err) return next_on_connect(err, req);
    auto read_err = temp_read_start();
    if (read_err) return next_on_connect(read_err, req);
    connect_request = req;
    start_ssl_connection(Profile::CLIENT);
}

void SSLFilter::accept (Stream* client) {
    _EDEBUG();

    client->async_lock();
    SSLFilter* client_filter = new SSLFilter(client, SSL_get_SSL_CTX(ssl));
    client->add_filter(client_filter);
    // set connecting status so that all other requests (write, etc) are put into queue until handshake completed
    client_filter->connecting(true);
    auto err = client_filter->temp_read_start();
    if (err) throw err;
    client_filter->start_ssl_connection(Profile::SERVER);
}

void SSLFilter::start_ssl_connection (Profile profile) {
    _EDEBUG("%s", PROFILE_STR);

    this->profile = profile;
    if (profile == Profile::CLIENT) SSL_set_connect_state(ssl);
    else                            SSL_set_accept_state(ssl);
    started = true;
    negotiate();
}

int SSLFilter::negotiate () {
    assert((!SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl)) && handle->connecting());
    int ssl_state = SSL_do_handshake(ssl);

    _EDEBUG("[%s] ssl_state=%d, renego pending %d", PROFILE_STR, ssl_state, SSL_renegotiate_pending(ssl));

    string write_buf = SSLBio::steal_buf(write_bio);
    if (write_buf) {
        _EDEBUG("[%s] writing %d bytes", PROFILE_STR, (int)write_buf.length());
        SSLWriteRequest* req = new SSLWriteRequest(nullptr);
        req->bufs.push_back(write_buf);
        next_write(req);
    }

    if (ssl_state <= 0) {
        int code = SSL_get_error(ssl, ssl_state);
        _EDEBUG("[%s] code=%d", PROFILE_STR, code);
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
//    if (!SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl) || !handle->connected()) throw CodeError(ERRNO_EINVAL);
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
    if (err) _EDEBUG("[%s] err=%s", PROFILE_STR, err->what());

    if (err && !handle->connecting()) return; // if not connecting then it's ongoing packets for already failed handshake, just ignore them
    restore_read_start(); // stop reading if handle don't want to
    if (profile == Profile::CLIENT) {
        next_on_connect(err, connect_request);
        connect_request = nullptr;
    }
    else {
        connecting(false);
        if (!err) connected(true);
        handle->call_on_ssl_connection(err);
        handle->async_unlock();
    }
}

void SSLFilter::on_read (const string& encbuf, const CodeError* err) {
    _EDEBUG("[%s], got %lu bytes", PROFILE_STR, encbuf.length());
    if (!handle->connecting() && !handle->connected()) {
        _EDEBUG("[%s], on_read strange state: neither connecting nor connected, possibly server is not configured", PROFILE_STR);
    }

    bool connecting = !SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl);
    if (err) {
        if (connecting) negotiation_finished(err);
        else next_on_read(encbuf, err);
        return;
    }

    SSLBio::set_buf(read_bio, encbuf);

    int pending = encbuf.length();
    if (connecting && !(pending = negotiate())) { // no more data to read, handshake not completed
        SSLBio::steal_buf(read_bio);
        return;
    }
    
    while (1) {
        string decbuf = handle->buf_alloc(pending);
        int ret = SSL_read(ssl, decbuf.buf(), pending);

        _EDEBUG("[%s], sslret=%d pending=%d", PROFILE_STR, ret, pending);
        
        if (ret > 0) {
            decbuf.length(ret);
#ifdef EVENT_LIB_SSL_DUMP_ON_READ
            _EDEBUG("[%s], data: %.*s", PROFILE_STR, (int)decbuf.length(), decbuf.data());
#endif
            next_on_read(decbuf, err);
        }
        else { 
            int ssl_code = SSL_get_error(ssl, ret);

            _EDEBUG("SSLFilter::on_read[%s], errno=%d, err=%d", PROFILE_STR, ssl_code, ERR_GET_LIB(ERR_peek_last_error()));
            
            if (ssl_code == SSL_ERROR_ZERO_RETURN || ssl_code == SSL_ERROR_WANT_READ) return;

            string wbuf = SSLBio::steal_buf(write_bio);
            if (wbuf) { // renegotiation
                _EDEBUG("[%s], write %lu", PROFILE_STR, wbuf.length());
                SSLWriteRequest* req = new SSLWriteRequest(nullptr);
                req->bufs.push_back(wbuf);
                next_write(req);
            }
            next_on_read(string(), SSLError(ssl_code));
            return;
        }
    }
}

void SSLFilter::write (WriteRequest* req) {
    _EDEBUG("[%s]", PROFILE_STR);

    assert(SSL_is_init_finished(ssl));
    assert(!handle->connecting());

    auto bufcnt = req->bufs.size();
    SSLWriteRequest* sslreq = new SSLWriteRequest(req);
    sslreq->retain();
    sslreq->bufs.reserve(bufcnt);

    for (size_t i = 0; i < bufcnt; i++) {
        if (req->bufs[i].length() == 0) {
            continue;
        }
        int res = SSL_write(ssl, req->bufs[i].data(), req->bufs[i].length());
        if (res <= 0) throw SSLError(SSL_get_error(ssl, res)); // TODO: handle renegotiation status
        string buf = SSLBio::steal_buf(write_bio);
        sslreq->bufs.push_back(buf);
    }

    next_write(sslreq);
}

void SSLFilter::on_write (const CodeError* err, WriteRequest* req) {
    SSLWriteRequest* sslreq = static_cast<SSLWriteRequest*>(req);
    _EDEBUG("[%s] regular=%d ERR=%s", PROFILE_STR, sslreq->src ? 1 : 0, err ? err->what() : "");
    if (sslreq->src) { // regular's on_write
        next_on_write(err, sslreq->src);
    } else { // negotiation's on_write
        if (err) negotiation_finished(err);
    }
    sslreq->release();
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

