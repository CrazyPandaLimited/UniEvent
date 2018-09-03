#include <panda/unievent/Stream.h>
#include <panda/unievent/StreamFilter.h>
#include <panda/unievent/SSLFilter.h>
#include <panda/unievent/Timer.h>
#include <chrono>
#include <panda/unievent/Prepare.h>
using namespace panda::unievent;

void Stream::uvx_on_connect (uv_connect_t* uvreq, int status) {
    ConnectRequest* r = rcast<ConnectRequest*>(uvreq);
    Stream* h = hcast<Stream*>(uvreq->handle);
    _EDEBUG("[%p] uvx_on_connect", h);
    CodeError err(status < 0 ? status : 0);
    if (!err && (h->flags & SF_WANTREAD)) err = h->_read_start();
    bool unlock = err.code() != ERRNO_ECANCELED;
    h->filter_list.head ? h->filter_list.head->on_connect(err, r) : h->call_on_connect(err, r, unlock);
}

void Stream::uvx_on_connection (uv_stream_t* stream, int status) {
    Stream* h = hcast<Stream*>(stream);
    CodeError err(status < 0 ? status : 0);
    h->call_on_connection(err);
}

void Stream::uvx_on_read (uv_stream_t* stream, ssize_t nread, const uv_buf_t* uvbuf) {
    Stream* h = hcast<Stream*>(stream);

    CodeError err(nread < 0 ? nread : 0);
    if (err.code() == ERRNO_EOF) {
        h->flags &= ~SF_CONNECTED;
        for (StreamFilter* f = h->filter_list.head; f; f = f->next()) f->on_eof();
        h->call_on_eof();
        nread = 0;
    }
    string buf;
    if (uvbuf->base) { // in some cases of eof it may be no buffer
        string* buf_ptr = (string*)(uvbuf->base + uvbuf->len);
        buf = *buf_ptr;
        buf_ptr->~string();
    }

    if (nread == 0) return; // UV just wants to release the buf

    buf.length(nread > 0 ? nread : 0); // set real buf len
    h->filter_list.head ? h->filter_list.head->on_read(buf, err) : h->call_on_read(buf, err);
}

void Stream::uvx_on_write (uv_write_t* uvreq, int status) {
    WriteRequest* r = rcast<WriteRequest*>(uvreq);
    Stream* h = hcast<Stream*>(uvreq->handle);
    CodeError err(status < 0 ? status : 0);
    h->filter_list.head ? h->filter_list.head->on_write(err, r) : h->call_on_write(err, r);
}

void Stream::uvx_on_shutdown (uv_shutdown_t* uvreq, int status) {
    ShutdownRequest* r = rcast<ShutdownRequest*>(uvreq);
    assert(!uv_is_closing(reinterpret_cast<uv_handle_t *>(uvreq->handle)));
    Stream* h = hcast<Stream*>(uvreq->handle);
    CodeError err(status < 0 ? status : 0);
    for (StreamFilter* f = h->filter_list.head; f; f = f->next()) f->on_shutdown(err, r);
    h->call_on_shutdown(err, r);
}

void Stream::listen (int backlog, connection_fn callback) {
    if (callback) connection_event.add(callback);
    int err = uv_listen(uvsp(), backlog, uvx_on_connection);
    if (err) throw CodeError(err);
}

void Stream::accept (Stream* client) {
    int uverr = uv_accept(uvsp(), client->uvsp());
    if (uverr) throw CodeError(uverr);
    client->flags |= SF_CONNECTED;
    for (StreamFilter* f = filter_list.head; f; f = f->next()) f->accept(client);
    if (client->flags & SF_WANTREAD) client->read_start();
}

void Stream::read_start (read_fn callback) {
    if (callback) read_event.add(callback);
    flags |= SF_WANTREAD;
    auto err = _read_start();
    if (err) throw err;
}

CodeError Stream::_read_start () {
    if (flags & SF_READING) return CodeError(); // uv_read_start has already been called
    os_fd_t fd;
    int fderr = uv_fileno(uvhp, &fd);
    if (fderr) return CodeError();
    int uverr = uv_read_start(uvsp(), Handle::uvx_on_buf_alloc, uvx_on_read);
    if (uverr) return CodeError(uverr);
    flags |= SF_READING;
    return CodeError();
}

void Stream::read_stop () {
    flags &= ~SF_WANTREAD;
    if (!(flags & SF_READING)) return; // uv_read_start has not been called
    flags &= ~SF_READING;
    uv_read_stop(uvsp());
}

void Stream::write (WriteRequest* req) {
    if (async_locked()) {
        asyncq_push(new CommandWrite(this, req));
        return;
    }

    _pex_(req)->handle = uvsp();
    req->retain();
    retain();
    filter_list.head ? filter_list.tail->write(req) : do_write(req);
}

void Stream::do_write (WriteRequest* req) {
    auto nbufs = req->bufs.size();
    uv_buf_t uvbufs[nbufs];
    uv_buf_t* ptr = uvbufs;
    for (const auto& str : req->bufs) {
        ptr->base = (char*)str.data();
        ptr->len  = str.length();
        ++ptr;
    }

    int err = uv_write(_pex_(req), uvsp(), uvbufs, nbufs, uvx_on_write);

    if (err) {
        Prepare::call_soon([=] {
            call_on_write(err, req);
        }, loop());
    }
}

void Stream::shutdown (ShutdownRequest* req) {
    if (!req) req = new ShutdownRequest();

    if (async_locked()) {
        asyncq_push(new CommandShutdown(this, req));
        return;
    }

    req->retain();
    int err = uv_shutdown(_pex_(req), uvsp(), uvx_on_shutdown);
    if (err) {
        Prepare::call_soon([=] {
            on_shutdown(err, req);
            req->release();
        }, loop());
        return;
    }

    flags |= SF_SHUTTING;
    async_lock();
    retain();
}

void Stream::disconnect () { close_reinit(asyncq_empty() && connecting() ? false : true); }
void Stream::reset      () { close_reinit(false); }

void Stream::on_handle_reinit () {
    for (StreamFilter* filter = filter_list.head; filter; filter = filter->next()) filter->reset();
    bool wanted_read = flags & SF_WANTREAD;
    Handle::on_handle_reinit();
    if (wanted_read) flags |= SF_WANTREAD;
}

void Stream::add_filter (StreamFilter* filter) {
    assert(filter);
    if (!filter_list.head) {
        filter_list.head = filter;
        filter->retain();
    }
    else filter_list.tail->next(filter);
    filter_list.tail = filter;
}

void Stream::use_ssl (SSL_CTX* context) {
    if (is_secure()) {
        return;
    }
    this->add_filter(new SSLFilter(this, context));
}

void Stream::use_ssl (const SSL_METHOD* method) {
    if (is_secure()) {
        return;
    }
    this->add_filter(new SSLFilter(this, method));
}

SSL* Stream::get_ssl () const {
    for (StreamFilter* filter = filter_list.head; filter; filter = filter->next())
        if (filter->type() == SSLFilter::TYPE) return static_cast<SSLFilter*>(filter)->get_ssl();
    return nullptr;
}

//void Stream::ssl_renegotiate () {
//    for (StreamFilter* filter = filter_list.head; filter; filter = filter->next())
//        if (filter->type() == SSLFilter::TYPE) {
//            static_cast<SSLFilter*>(filter)->renegotiate();
//            return;
//        }
//}

bool Stream::is_secure () const {
    for (StreamFilter* filter = filter_list.head; filter; filter = filter->next())
        if (filter->is_secure()) return true;
    return false;
}

void Stream::call_on_connect (const CodeError& err, ConnectRequest* req, bool unlock) {
    flags &= ~SF_CONNECTING;
    if (!err) flags |= SF_CONNECTED;
    req->release_timer();
    if (asyncq_empty()) {
        if (unlock) async_unlock_noresume();
        {
            auto guard = lock_in_callback();
            req->event(this, err, req);
            on_connect(err, req);
        }
        req->release();
    } else {
        CommandBase* last_tail = asyncq.tail;
        {
            auto guard = lock_in_callback();
            req->event(this, err, req);
            on_connect(err, req);
        }
        req->release();
        if (err) asyncq_cancel_connect(last_tail); // remove writes, shutdowns, etc - till first disconnect - only if no reconnect
        if (unlock) async_unlock();
    }
    release();
}

void Stream::cancel_connect() {
    _close();
}
void Stream::asyncq_cancel_connect (CommandBase* last_tail) {
    if (asyncq.tail != last_tail) { // reconnect possible
        // now ensure that last_tail is still in async queue (reset hasn't been called)
        bool found = false;
        for (const CommandBase* cmd = asyncq.head; cmd; cmd = cmd->next) if (cmd == last_tail) {
            found = true;
            break;
        }
        if (found) {
            found = false;
            for (const CommandBase* cmd = last_tail->next; cmd; cmd = cmd->next) {
                if (cmd->type == CommandBase::Type::CLOSE_REINIT || cmd->type == CommandBase::Type::SHUTDOWN ||
                    cmd->type == CommandBase::Type::USER_CALLBACK) continue; // allowed before reconnect
                if (cmd->type != CommandBase::Type::CONNECT || !static_cast<const CommandConnect*>(cmd)->is_reconnect()) break; // no reconnect
                found = true;
                break;
            }
        }
        if (found) { // reconnect found, move everything below last_tail to the top of async queue
            asyncq.tail->next = asyncq.head;
            asyncq.head = last_tail->next;
            last_tail->next = nullptr;
            return;
        }
    }

    // reconnect not found -> clear write requests, shutdowns, etc till first disconnect
    while (asyncq.head && asyncq.head->type != CommandBase::Type::CLOSE_DELETE && asyncq.head->type != CommandBase::Type::CLOSE_REINIT) {
        CommandBase* cmd = asyncq.head;
        asyncq.head = cmd->next;
        cmd->cancel();
        delete cmd;
    }
}

void Stream::on_connection (const CodeError& err) {
    if (connection_event.has_listeners()) connection_event(this, err);
    else throw ImplRequiredError("Stream::on_connection");
}

void Stream::on_ssl_connection (const CodeError& err) {
    ssl_connection_event(this, err);
}

void Stream::on_connect (const CodeError& err, ConnectRequest* req) {
    connect_event(this, err, req);
}

void Stream::on_read (const string& buf, const CodeError& err) {
    if (read_event.has_listeners()) read_event(this, buf, err);
}

void Stream::on_write (const CodeError& err, WriteRequest* req) {
    write_event(this, err, req);
}

void Stream::on_shutdown (const CodeError& err, ShutdownRequest* req) {
    shutdown_event(this, err, req);
}

void Stream::on_eof () {
    eof_event(this);
}

Stream::~Stream () {
    if (filter_list.head) filter_list.head->release();
}
