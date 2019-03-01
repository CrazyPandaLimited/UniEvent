#include <chrono>
#include <algorithm>

#include "Timer.h"
#include "Stream.h"
#include "Prepare.h"
#include "StreamFilter.h"
#include "ssl/SSLFilter.h"

namespace panda { namespace unievent {

Stream::~Stream () {
    _EDTOR();
}

Stream::Stream () : filters_(this) {
    _ECTOR();
}

void Stream::uvx_on_connect (uv_connect_t* uvreq, int status) {
    ConnectRequest* r = rcast<ConnectRequest*>(uvreq);
    Stream* h = hcast<Stream*>(uvreq->handle);
    _EDEBUG("[%p] uvx_on_connect, req: %p", h, r);
    CodeError err(status);
    if (!err && (h->wantread())) err = h->_read_start();
    h->filters_.on_connect(err, r);
}

void Stream::do_on_connect (const CodeError* err, ConnectRequest* connect_request) {
    _EDEBUGTHIS("do_on_connect, err: %d, stream: %p, request: %p", err ? err->code() : 0, this, connect_request);
//    panda_log_debug(static_cast<Handle*>(this) << (err ? err->what() : "NO_ERROR") <<":" << connect_request);
    bool unlock = !(err && err->code() == ERRNO_ECANCELED);
    set_connected(!err);
    connect_request->release_timer();
    if (asyncq_empty()) {
        if (unlock) {
            async_unlock_noresume();
        }
        {
            auto guard = lock_in_callback();
            connect_request->event(this, err, connect_request);
            on_connect(err, connect_request);
        }
        connect_request->release();
    } else {
        CommandBase* last_tail = asyncq.tail;
        {
            auto guard = lock_in_callback();
            connect_request->event(this, err, connect_request);
            on_connect(err, connect_request);
        }
        connect_request->release();
        if (err) asyncq_cancel_connect(last_tail); // remove writes, shutdowns, etc - till first disconnect - only if no reconnect
        if (unlock) async_unlock();
    }
    release();
}

void Stream::uvx_on_connection (uv_stream_t* stream, int status) {
    Stream* h = hcast<Stream*>(stream);
    _EDEBUG("[%p] uvx_on_connection, err: %d", h, status);
    if (status) h->filters_.on_connection(nullptr, CodeError(status));
    else        h->accept();
}

void Stream::do_on_connection (StreamSP stream, const CodeError* err) {
    _EDEBUGTHIS("do_on_connection, err: %d, stream: %p", err ? err->code() : 0, stream.get());
    stream->set_connected(!err);
    {
        auto guard = stream->lock_in_callback();
        on_connection(stream, err);
    }
}

void Stream::uvx_on_read (uv_stream_t* stream, ssize_t nread, const uv_buf_t* uvbuf) {
    Stream* h = hcast<Stream*>(stream);
    _EDEBUG("[%p] uvx_on_read, nread: %ld", h, nread);
    CodeError err(nread < 0 ? nread : 0);
    if (err.code() == ERRNO_EOF) {
        h->filters_.on_eof();
        nread = 0;
    }

    // in some cases of eof it may be no buffer
    string buf;
    if (uvbuf->base) { 
        string* buf_ptr = (string*)(uvbuf->base + uvbuf->len);
        buf = *buf_ptr;
        buf_ptr->~string();
    }

    // UV just wants to release the buf
    if (nread == 0) {
        return;
    }

    buf.length(nread > 0 ? nread : 0); // set real buf len
    h->filters_.on_read(buf, err);
}

void Stream::uvx_on_write (uv_write_t* uvreq, int status) {
    WriteRequest* r = rcast<WriteRequest*>(uvreq);
    Stream* h = hcast<Stream*>(uvreq->handle);
    _EDEBUG("[%p] uvx_on_write, err: %d", h, status);
    h->filters_.on_write(CodeError(status), r);
}

void Stream::do_on_write (const CodeError* err, WriteRequest* write_request) {
    _EDEBUGTHIS("on_write, err: %d, handle: %p, request: %p", err ? err->code() : 0, this, write_request);
    {
        auto guard = lock_in_callback();
        write_request->event(this, err, write_request);
        on_write(err, write_request);
    }
    write_request->release();
    release();
}

void Stream::uvx_on_shutdown (uv_shutdown_t* uvreq, int status) {
    //assert(!uv_is_closing(reinterpret_cast<uv_handle_t *>(uvreq->handle))); // COMMENTED OUT - TO REFACTOR - иначе падают тесты Front. ¬озникает когда после shutdown() сказали reset()
    ShutdownRequest* r = rcast<ShutdownRequest*>(uvreq);
    Stream* h = hcast<Stream*>(uvreq->handle);
    _EDEBUG("[%p] uvx_on_shutdown, err: %d", h, status);
    h->filters_.on_shutdown(CodeError(status), r);
}

void Stream::do_on_shutdown (const CodeError* err, ShutdownRequest* shutdown_request) {
    _EDEBUGTHIS("on_shutdown");
    bool unlock = !(err && err->code() == ERRNO_ECANCELED);
    set_shutdown(!err);
    {
        auto guard = lock_in_callback();
        shutdown_request->event(this, err, shutdown_request);
        on_shutdown(err, shutdown_request);
    }
    shutdown_request->release();
    if (unlock) async_unlock();
    release();
}

void Stream::listen (int backlog, connection_fn callback) {
    auto filter = get_filter<ssl::SSLFilter>();
    if (filter && filter->is_client()) throw Error("Programming error, use server certificate");
    if (callback) connection_event.add(callback);
    int err = uv_listen(uvsp(), backlog, uvx_on_connection);
    if (err) throw CodeError(err);
    flags |= SF_LISTENING;
}

void Stream::accept () {
    accept(connection_factory ? connection_factory() : on_create_connection());
}

void Stream::accept (const StreamSP& stream) {
    _EDEBUGTHIS("accept %p", stream.get());
    stream->retain();
    // set connecting status so that all other requests (write, etc) are put into queue until handshake completed
    stream->set_connecting();
    int err = uv_accept(uvsp(), stream->uvsp());
    filters_.on_connection(stream, CodeError(err));
    stream->release();
}

void Stream::read_start (read_fn callback) {
    if (callback) read_event.add(callback);
    set_wantread();
    auto err = _read_start();
    if (err) throw err;
}

CodeError Stream::_read_start () {
    if (reading()) return CodeError(); // uv_read_start has already been called
    os_fd_t fd;
    int fderr = uv_fileno(uvhp, &fd);
    if (fderr) return CodeError();
    int uverr = uv_read_start(uvsp(), Handle::uvx_on_buf_alloc, uvx_on_read);
    if (uverr) return CodeError(uverr);
    set_reading();
    return CodeError();
}

void Stream::read_stop () {
    clear_wantread();
    if (!reading()) return; // uv_read_start has not been called
    clear_reading();
    uv_read_stop(uvsp());
}

void Stream::write (WriteRequest* req) {
    _EDEBUGTHIS("write, locked %d", (int)async_locked());
    if (async_locked()) {
        asyncq_push(new CommandWrite(this, req));
        return;
    }
    
    _pex_(req)->handle = uvsp();
    req->retain();
    retain();

    filters_.write(req);
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
            filters_.on_write(CodeError(err), req);
        }, loop());
    }
}

void Stream::shutdown (ShutdownRequest* req) {
    _EDEBUGTHIS("shutdown, req: %p, locked: %d", req, (int)async_locked());
    if (!req) req = new ShutdownRequest();

    if (async_locked()) {
        asyncq_push(new CommandShutdown(this, req));
        return;
    }

    req->retain();
    set_shutting();
    async_lock();
    retain();
    
    int err = uv_shutdown(_pex_(req), uvsp(), uvx_on_shutdown);
    if (err) {
        Prepare::call_soon([=] {
            filters_.on_shutdown(CodeError(err), req);
        }, loop());
        return;
    }

}

void Stream::disconnect () { close_reinit(asyncq_empty() && connecting() ? false : true); }
void Stream::reset      () { close_reinit(false); }

void Stream::on_handle_reinit () {
    _EDEBUGTHIS("on_handle_reinit");
    filters_.on_reinit();
    bool wanted_read = wantread();
    Handle::on_handle_reinit();
    if (wanted_read) set_wantread();
}

void Stream::add_filter (const StreamFilterSP& filter) {
    assert(filter);
    auto it = filters_.begin();
    auto pos = it;
    bool found = false;
    while (it != filters_.end()) {
        if ((*it)->type() == filter->type()) {
            *it = filter;
            return;
        }
        if ((*it)->priority() > filter->priority() && !found) {
            pos = it;
            found = true;
        }
        it++;
    }
    if (found) filters_.insert(pos, filter);
    else filters_.push_back(filter);
}

StreamFilterSP Stream::get_filter (const void* type) const {
    for (const auto& f : filters_) if (f->type() == type) return f;
    return {};
}

void Stream::use_ssl (SSL_CTX* context) { add_filter(new ssl::SSLFilter(this, context)); }

void Stream::use_ssl (const SSL_METHOD* method) {
    ssl::SSLFilterSP f = new ssl::SSLFilter(this, method);
    if (listening() && f->is_client()) throw Error("Using ssl without certificate on listening stream");
    add_filter(f);
}

bool Stream::is_secure () const { return get_filter(ssl::SSLFilter::TYPE); }

SSL* Stream::get_ssl () const {
    auto filter = get_filter<ssl::SSLFilter>();
    return filter ? filter->get_ssl() : nullptr;
}

void Stream::_close () {
    Handle::_close();
}

void Stream::cancel_connect () {
    _EDEBUGTHIS("cancel_connect");
    _close();
}

void Stream::asyncq_cancel_connect (CommandBase* last_tail) {
    _EDEBUGTHIS("asyncq_cancel_connect");
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

void Stream::on_connection (StreamSP stream, const CodeError* err) {
    connection_event(this, stream, err);
}

void Stream::on_connect (const CodeError* err, ConnectRequest* req) {
    connect_event(this, err, req);
}

void Stream::on_read (string& buf, const CodeError* err) {
    if (read_event.has_listeners()) read_event(this, buf, err);
}

void Stream::on_write (const CodeError* err, WriteRequest* req) {
    write_event(this, err, req);
}

void Stream::on_shutdown (const CodeError* err, ShutdownRequest* req) {
    shutdown_event(this, err, req);
}

void Stream::on_eof () {
    eof_event(this);
}
    
StreamSP Stream::on_create_connection () {
    throw ImplRequiredError("on_create_connection");
}

}}
