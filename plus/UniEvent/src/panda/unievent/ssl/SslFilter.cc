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

namespace panda { namespace unievent { namespace ssl {

log::Module ssllog("UniEvent::SSL", log::Warning);
static log::Module* panda_log_module = &ssllog;

#define _ESSL(fmt, ...) do { \
    char _log_buf_[1000]; \
    int _log_size_ = snprintf(_log_buf_, 1000, "%s(): [%s] {%p} " fmt "\n", __func__, profile == Profile::CLIENT ? "client" : (profile == Profile::SERVER ? "server" : "no profile"), this->handle, ##__VA_ARGS__); \
    panda_mlog_debug(ssllog, string_view(_log_buf_, _log_size_)); \
} while(0)

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

static inline SSL_CTX* ssl_ctx_from_method (const SSL_METHOD* method) {
    if (!method) method = SSLv23_client_method();
    auto context = SSL_CTX_new(method);
    if (!context) throw SSLError(SSL_ERROR_SSL);
    return context;
}

struct SslWriteRequest : WriteRequest {
    WriteRequest* orig;
    bool          final;
    SslWriteRequest (WriteRequest* req = nullptr) : WriteRequest(), orig(req), final() {}
};
using SslWriteRequestSP = iptr<SslWriteRequest>;

SslFilter::SslFilter (Stream* stream, SSL_CTX* context, const SslFilterSP& server_filter)
        : StreamFilter(stream, TYPE, PRIORITY), state(State::initial), profile(Profile::UNKNOWN), server_filter(server_filter)
{
    _ECTOR();
    if (stream->listening() && !SSL_CTX_check_private_key(context)) throw Error("SSL certificate&key needed to listen()");
    init(context);
}

SslFilter::SslFilter (Stream* stream, const SSL_METHOD* method) : SslFilter(stream, ssl_ctx_from_method(method)) {
    SSL_CTX_free(SSL_get_SSL_CTX(ssl)); // it is refcounted, release ctx created from ssl_ctx_from_method
}

SslFilter::~SslFilter () {
    _EDTOR();
    SSL_free(ssl);
}

void SslFilter::init (SSL_CTX* context) {
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

void SslFilter::listen () {
    if (!SSL_check_private_key(ssl)) throw Error("SSL certificate&key needed to listen()");
    NextFilter::listen();
}

void SslFilter::reset () {
    if (state == State::initial) return;
    _ESSL("reset, state: %d, connecting: %d", (int)state, handle->connecting());

    source_request = nullptr;
    ERR_clear_error();
    state = State::initial;

    // hard reset
    SSL* oldssl = ssl;
    init(SSL_get_SSL_CTX(oldssl));
    SSL_free(oldssl);
    //// soft reset - openssl docs say it should work, but IT DOES NOT WORK!
    //if (!SSL_clear(ssl)) throw SSLError(SSL_ERROR_SSL);
    NextFilter::reset();
}

void SslFilter::handle_connect (const CodeError& err, const ConnectRequestSP& req) {
    _ESSL("ERR=%s", err.what());
    //if (state == State::terminal) {
        // we need this for ssl filters chaining
        //NextFilter::on_connect(err, req);
        //return;
    //}

    reset();

    if (err) return NextFilter::handle_connect(err, req);

    auto read_err = read_start();
    if (read_err) return NextFilter::handle_connect(read_err, req);

    source_request = req;
    start_ssl_connection(Profile::CLIENT);
}

void SslFilter::handle_connection (const StreamSP& client, const CodeError& err, const AcceptRequestSP& req) {
    _ESSL("client: %p, err: %s", client.get(), err.what());
    if (err) return NextFilter::handle_connection(client, err, req);

    SslFilter* filter = new SslFilter(client, SSL_get_SSL_CTX(ssl), this);
    client->add_filter(filter);

    auto read_err = filter->read_start();
    if (read_err) return NextFilter::handle_connection(client, read_err, req);

    assert(req);
    filter->source_request = req;
    filter->start_ssl_connection(Profile::SERVER);
}

void SslFilter::start_ssl_connection (Profile profile) {
    _ESSL();

    this->profile = profile;
    if (profile == Profile::CLIENT) SSL_set_connect_state(ssl);
    else                            SSL_set_accept_state(ssl);

    negotiate();
}

int SslFilter::negotiate () {
    bool renegotiate = SSL_renegotiate_pending(ssl);
    assert((!SSL_is_init_finished(ssl) && handle->connecting()) || renegotiate);

    state = State::negotiating;

    int ssl_state = SSL_do_handshake(ssl);

    _ESSL("ssl_state=%d, renego pending %d", ssl_state, SSL_renegotiate_pending(ssl));

    if (ssl_state <= 0) {
        int code = SSL_get_error(ssl, ssl_state);
        _ESSL("code=%d", code);
        if (code != SSL_ERROR_WANT_READ && code != SSL_ERROR_WANT_WRITE) {
            negotiation_finished(SSLError(code));
            return 0;
        }
    }

    string write_buf = SslBio::steal_buf(write_bio);
    SslWriteRequestSP wreq;
    if (write_buf) {
        _ESSL("writing %d bytes", (int)write_buf.length());
        wreq = new SslWriteRequest();
        wreq->bufs.push_back(write_buf);
    }

    if (SSL_is_init_finished(ssl)) {
        auto pending = BIO_ctrl(read_bio, BIO_CTRL_PENDING, 0, nullptr);
        _ESSL("ssl finished, pending = %li", pending);
        if (wreq) { // negotiation finished on my last write -> call negotiation_finished() later with results for wreq
            wreq->final = true;
            subreq_write(source_request, wreq);
            return 0;
        } else {
            negotiation_finished();
            return pending;
        }
    }

    if (wreq) subreq_write(source_request, wreq);
    return 0;
}

//// renegotiate DOES NOT WORK - SSL_is_init_finshed returns true, SSL_renegotiate_pending becomes false after first SSL_do_handshake
//void SslFilter::renegotiate () {
//    printf("SslFilter::renegotiate\n");
//    if (!SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl) || !handle->connected()) throw StreamError(ERRNO_EINVAL);
//    SSL_set_verify(ssl, SSL_VERIFY_PEER, nullptr);
//    printf("SSL_get_secure_renegotiation_support=%li\n", SSL_get_secure_renegotiation_support(ssl));
//    int ssl_state = SSL_renegotiate(ssl);
//    printf("SslFilter::renegotiate: renego pending %d\n", SSL_renegotiate_pending(ssl));
//    if (ssl_state <= 0) {
//        int code = SSL_get_error(ssl, ssl_state);
//        throw SSLError(code);
//    }
//    handle->connected(false);
//    handle->connecting(true);
//    negotiate();
//}

void SslFilter::negotiation_finished (const CodeError& err) {
    _ESSL("connecting: %d err=%s", (int)handle->connecting(), err.what());

    if (state == State::terminal || state == State::error) return;

    if (handle->connected()) { // prevent double callback call after renegotiate
        state = State::terminal;
        return;
    }

    state = err ? State::error : State::terminal;

    read_stop();

    auto tmp = std::move(source_request);
    if (profile == Profile::CLIENT)
        NextFilter::handle_connect(err, static_pointer_cast<ConnectRequest>(tmp));
    else if (auto f = server_filter.lock())
        f->NextFilter::handle_connection(handle, err, static_pointer_cast<AcceptRequest>(tmp));
}

void SslFilter::handle_read (string& encbuf, const CodeError& err) {
    _ESSL("got %lu bytes, state: %d", encbuf.length(), (int)state);
    if (state == State::error) {
        NextFilter::handle_read(encbuf, SSLError(SSL_ERROR_SSL));
        return;
    }

    assert(handle->connecting() || handle->connected());

    panda_log_debug("ssl_init_finished" << SSL_is_init_finished(ssl) << ", renegotiate " << SSL_renegotiate_pending(ssl));
    bool connecting = !SSL_is_init_finished(ssl) || SSL_renegotiate_pending(ssl);
    if (err) {
        if (!handle->connecting()) {
            // if not connecting then it's ongoing packets for already failed handshake, just ignore them
        }
        else if (connecting) {
            negotiation_finished(err);
        }
        else {
            NextFilter::handle_read(encbuf, err);
        }
        return;
    }
    SslBio::set_buf(read_bio, encbuf);

    panda_log_debug("connecting " << connecting << ", err " << err.what());

    auto was_in_read = BIO_ctrl(read_bio, BIO_CTRL_PENDING, 0, nullptr);
    int pending = was_in_read + encbuf.length();
    if (connecting) pending = negotiate();
    if (!pending) return;

    if (state == State::negotiating) { // SSL_is_init_finished but we are waiting for handle_write to finish negotiation
        // data has alrready collected by SslBio::set_buf(read_bio, encbuf);
        return;
    }

    // TODO: prevent buf_alloc for last fake read (when -1 returned)
    int ret;
    string decbuf = handle->buf_alloc(pending);
    while (1) {
        if (state == State::initial) return; // handle has been reset in negotiate() or handle_read()
        if (decbuf.use_count() > 1) decbuf = handle->buf_alloc(pending); // or it will detach with default allocator
        if (decbuf.capacity() < (unsigned)pending) decbuf.reserve(pending); // TODO: 1) handle cap=0 via ENOMEM, 2) handle (cap < pending) better (multi-alloc)
        ret = SSL_read(ssl, decbuf.buf(), pending);
        if (ret <= 0) break;
        decbuf.length(ret);
        NextFilter::handle_read(decbuf, err);
        if (!handle->wantread()) {
            return;
        }
    }

    int ssl_code = SSL_get_error(ssl, ret);
    _ESSL("errno=%d, err=%d", ssl_code, ERR_GET_LIB(ERR_peek_last_error()));

    if (ssl_code == SSL_ERROR_ZERO_RETURN || ssl_code == SSL_ERROR_WANT_READ) return;

    if (ssl_code == SSL_ERROR_WANT_WRITE) { // renegotiation
        string wbuf = SslBio::steal_buf(write_bio);
        _ESSL("write %lu", wbuf.length());
        WriteRequestSP req = new SslWriteRequest();
        req->bufs.push_back(wbuf);
        subreq_write(source_request, req);
    } else {
        string s;
        NextFilter::handle_read(s, SSLError(ssl_code));
    }
}

void SslFilter::write (const WriteRequestSP& req) {
    assert(state == State::terminal);

    WriteRequestSP sslreq = new SslWriteRequest(req);
    _ESSL("request: %p, sslrequest: %p", req.get(), sslreq.get());

    auto bufcnt = req->bufs.size();
    sslreq->bufs.reserve(bufcnt);
    for (size_t i = 0; i < bufcnt; i++) {
        if (req->bufs[i].length() == 0) continue;
        int res = SSL_write(ssl, req->bufs[i].data(), req->bufs[i].length());
        if (res <= 0) {
            // TODO: handle renegotiation status
            _ESSL("ssl failed");
            SSLError error(SSL_ERROR_SSL);
            req->delay([weak_req=req.get(), error, this]{ NextFilter::handle_write(error, weak_req); });
            return;
        }
        string buf = SslBio::steal_buf(write_bio);
        sslreq->bufs.push_back(buf);
    }

    subreq_write(req, sslreq);
}

void SslFilter::handle_write (const CodeError& err, const WriteRequestSP& req) {
    auto reqp = req.get();
    assert(typeid(*reqp) == typeid(SslWriteRequest));
    subreq_done(req);
    auto sslreq = static_cast<SslWriteRequest*>(reqp);

    _ESSL("state=%d request=%p regular=%d ERR=%s", (int)state, sslreq, sslreq->orig ? 1 : 0, err.what());
    if (sslreq->orig) { // regular write
        NextFilter::handle_write(err, sslreq->orig);
    }
    else { // negotiation
        if (err) return negotiation_finished(err);
        if (sslreq->final) {
            negotiation_finished(); // delayed negotiation_finished() for server with the results of last write request
            auto has_messge = BIO_ctrl(read_bio, BIO_CTRL_PENDING, 0, nullptr);
            if (has_messge) {
                string fake;
                handle_read(fake, {});
            }
        }
    }
}

//void SslFilter::on_reinit () {
//    _ESSL("on_reinit, state: %d, connecting: %d", (int)state, handle->connecting());
//    if (state == State::negotiating) {
//        negotiation_finished(CodeError(ERRNO_ECANCELED));
//    } else if (state == State::terminal) {
//        NextFilter::on_reinit();
//    }
//}

void SslFilter::handle_eof () {
    _ESSL("state: %d", (int)state);
    if (state == State::terminal) {
        NextFilter::handle_eof();
    }
    else if (state == State::negotiating) {
        negotiation_finished(std::errc::connection_aborted);
    }
}

}}}
