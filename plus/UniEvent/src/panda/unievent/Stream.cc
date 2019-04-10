#include "Stream.h"
//#include "Timer.h"
//#include "Prepare.h"
//#include "ssl/SSLFilter.h"
//#include <chrono>
//#include <algorithm>

using namespace panda::unievent;

Stream::~Stream () {
    _EDTOR();
}

string Stream::buf_alloc (size_t cap) noexcept {
    try {
        return buf_alloc_callback ? buf_alloc_callback(cap) : string(cap);
    } catch (...) {
        return {};
    }
}

void Stream::listen (connection_fn callback, int backlog) {
//    auto filter = get_filter<ssl::SSLFilter>();
//    if (filter && filter->is_client()) throw Error("Programming error, use server certificate");
    if (callback) connection_event.add(callback);
    impl()->listen(backlog);
    set_listening();
}

void Stream::handle_connection (const CodeError* err) {
    _EDEBUG("[%p] err: %d", this, err ? err->code().value() : 0);
    if (err) invoke(_filters.back(), &StreamFilter::handle_connection, &Stream::finalize_handle_connection, nullptr, err);
    else     accept();
}

void Stream::accept () {
    accept(connection_factory ? connection_factory() : create_connection());
}

void Stream::accept (const StreamSP& client) {
    _EDEBUGTHIS("client=%p", client.get());
    auto err = impl()->accept(client->impl());
    if (!err) client->set_connecting();
    invoke(_filters.back(), &StreamFilter::handle_connection, &Stream::finalize_handle_connection, client, err);
}

void Stream::finalize_handle_connection (const StreamSP& client, const CodeError* err) {
    _EDEBUGTHIS("err: %d, client: %p", err ? err->code().value() : 0, client.get());
    client->set_connected(!err);
    on_connection(client, err);
}

void Stream::on_connection (const StreamSP& client, const CodeError* err) {
    connection_event(this, client, err);
}

void ConnectRequest::exec () {
    _EDEBUGTHIS();
    handle->set_connecting();

    if (timeout) {
        timer = new Timer(handle->loop());
        timer->event.add([this](const TimerSP&){
            handle_connect(CodeError(std::errc::timed_out));
        });
        timer->once(timeout);
    }
}

void ConnectRequest::handle_connect (const CodeError* err) {
    _EDEBUGTHIS();
    handle->invoke(handle->_filters.back(), &StreamFilter::handle_connect, &Stream::finalize_handle_connect, err, this);
}

void Stream::finalize_handle_connect (const CodeError* err, const ConnectRequestSP& req) {
    _EDEBUGTHIS("err: %d, request: %p", err ? err->code().value() : 0, req.get());

    req->timer = nullptr;
    set_connected(!err);

    if (!err) {
        queue.done(req, [&]{
            req->event(this, err, req);
            on_connect(err, req);
        });
    }
    else {
        queue.cancel([&]{
            queue.done(req, [&]{
                req->event(this, err, req);
                on_connect(err, req);
            });
        }, [&]{
            do_reset();
        });
    }
}

void ConnectRequest::cancel () {
    handle_connect(CodeError(std::errc::operation_canceled));
}

void Stream::on_connect (const CodeError* err, const ConnectRequestSP& req) {
    connect_event(this, err, req);
}

//ConnectRequest::~ConnectRequest() {
//    release_timer();
//}
//
//void ConnectRequest::set_timer(Timer* timer) {
//    timer_ = timer;
//    timer_->retain();
//    _EDEBUGTHIS("set timer %p", timer_);
//}
//
//void ConnectRequest::release_timer() {
//    _EDEBUGTHIS("%p", timer_);
//    if(timer_) {
//        timer_->stop();
//        timer_->release();
//        timer_ = nullptr;
//    }
//}

//void Stream::uvx_on_connect (uv_connect_t* uvreq, int status) {
//    ConnectRequest* r = rcast<ConnectRequest*>(uvreq);
//    Stream* h = hcast<Stream*>(uvreq->handle);
//    _EDEBUG("[%p] uvx_on_connect, req: %p", h, r);
//    CodeError err(status);
//    if (!err && (h->wantread())) err = h->_read_start();
//    h->filters_.on_connect(err, r);
//}
//

//
//void Stream::uvx_on_read (uv_stream_t* stream, ssize_t nread, const uv_buf_t* uvbuf) {
//    Stream* h = hcast<Stream*>(stream);
//    _EDEBUG("[%p] uvx_on_read, nread: %ld", h, nread);
//    CodeError err(nread < 0 ? nread : 0);
//    if (err.code() == ERRNO_EOF) {
//        h->filters_.on_eof();
//        nread = 0;
//    }
//
//    // in some cases of eof it may be no buffer
//    string buf;
//    if (uvbuf->base) {
//        string* buf_ptr = (string*)(uvbuf->base + uvbuf->len);
//        buf = *buf_ptr;
//        buf_ptr->~string();
//    }
//
//    // UV just wants to release the buf
//    if (nread == 0) {
//        return;
//    }
//
//    buf.length(nread > 0 ? nread : 0); // set real buf len
//    h->filters_.on_read(buf, err);
//}
//
//void Stream::uvx_on_write (uv_write_t* uvreq, int status) {
//    WriteRequest* r = rcast<WriteRequest*>(uvreq);
//    Stream* h = hcast<Stream*>(uvreq->handle);
//    _EDEBUG("[%p] uvx_on_write, err: %d", h, status);
//    h->filters_.on_write(CodeError(status), r);
//}
//
//void Stream::do_on_write (const CodeError* err, WriteRequest* write_request) {
//    _EDEBUGTHIS("on_write, err: %d, handle: %p, request: %p", err ? err->code() : 0, this, write_request);
//    {
//        auto guard = lock_in_callback();
//        write_request->event(this, err, write_request);
//        on_write(err, write_request);
//    }
//    write_request->release();
//    release();
//}
//
//void Stream::uvx_on_shutdown (uv_shutdown_t* uvreq, int status) {
//    //assert(!uv_is_closing(reinterpret_cast<uv_handle_t *>(uvreq->handle))); // COMMENTED OUT - TO REFACTOR - ����� ������ ����� Front. ��������� ����� ����� shutdown() ������� reset()
//    ShutdownRequest* r = rcast<ShutdownRequest*>(uvreq);
//    Stream* h = hcast<Stream*>(uvreq->handle);
//    _EDEBUG("[%p] uvx_on_shutdown, err: %d", h, status);
//    h->filters_.on_shutdown(CodeError(status), r);
//}
//
//void Stream::do_on_shutdown (const CodeError* err, ShutdownRequest* shutdown_request) {
//    _EDEBUGTHIS("on_shutdown");
//    bool unlock = !(err && err->code() == ERRNO_ECANCELED);
//    set_shutdown(!err);
//    {
//        auto guard = lock_in_callback();
//        shutdown_request->event(this, err, shutdown_request);
//        on_shutdown(err, shutdown_request);
//    }
//    shutdown_request->release();
//    if (unlock) async_unlock();
//    release();
//}
//void Stream::read_start (read_fn callback) {
//    if (callback) read_event.add(callback);
//    set_wantread();
//    auto err = _read_start();
//    if (err) throw err;
//}
//
//CodeError Stream::_read_start () {
//    if (reading()) return CodeError(); // uv_read_start has already been called
//    os_fd_t fd;
//    int fderr = uv_fileno(uvhp, &fd);
//    if (fderr) return CodeError();
//    int uverr = uv_read_start(uvsp(), Handle::uvx_on_buf_alloc, uvx_on_read);
//    if (uverr) return CodeError(uverr);
//    set_reading();
//    return CodeError();
//}
//
//void Stream::read_stop () {
//    clear_wantread();
//    if (!reading()) return; // uv_read_start has not been called
//    clear_reading();
//    uv_read_stop(uvsp());
//}
//
//void Stream::write (WriteRequest* req) {
//    _EDEBUGTHIS("write, locked %d", (int)async_locked());
//    if (async_locked()) {
//        asyncq_push(new CommandWrite(this, req));
//        return;
//    }
//
//    _pex_(req)->handle = uvsp();
//    req->retain();
//    retain();
//
//    filters_.write(req);
//}
//
//void Stream::do_write (WriteRequest* req) {
//    auto nbufs = req->bufs.size();
//    uv_buf_t uvbufs[nbufs];
//    uv_buf_t* ptr = uvbufs;
//    for (const auto& str : req->bufs) {
//        ptr->base = (char*)str.data();
//        ptr->len  = str.length();
//        ++ptr;
//    }
//
//    int err = uv_write(_pex_(req), uvsp(), uvbufs, nbufs, uvx_on_write);
//    if (err) {
//        Prepare::call_soon([=] {
//            filters_.on_write(CodeError(err), req);
//        }, loop());
//    }
//}
//
//void Stream::shutdown (ShutdownRequest* req) {
//    _EDEBUGTHIS("shutdown, req: %p, locked: %d", req, (int)async_locked());
//    if (!req) req = new ShutdownRequest();
//
//    if (async_locked()) {
//        asyncq_push(new CommandShutdown(this, req));
//        return;
//    }
//
//    req->retain();
//    set_shutting();
//    async_lock();
//    retain();
//
//    int err = uv_shutdown(_pex_(req), uvsp(), uvx_on_shutdown);
//    if (err) {
//        Prepare::call_soon([=] {
//            filters_.on_shutdown(CodeError(err), req);
//        }, loop());
//        return;
//    }
//
//}
//
//void Stream::disconnect () { close_reinit(asyncq_empty() && connecting() ? false : true); }

void Stream::reset () {
    queue.cancel([&]{ do_reset(); });
}

void Stream::do_reset () {
    Handle::reset();
    if (_filters.size()) _filters.front()->reset();
    flags &= WANTREAD; // clear flags except WANTREAD
}

void Stream::clear () {
    queue.cancel([&]{
        Handle::clear();
        flags = WANTREAD;
        buf_alloc_callback = nullptr;
        connection_factory = nullptr;
        connection_event.remove_all();
        connect_event.remove_all();
        read_event.remove_all();
        write_event.remove_all();
        shutdown_event.remove_all();
        eof_event.remove_all();
    });
}

//void Stream::on_handle_reinit () {
//    _EDEBUGTHIS("on_handle_reinit");
//    filters_.on_reinit();
//    bool wanted_read = wantread();
//    Handle::on_handle_reinit();
//    if (wanted_read) set_wantread();
//}
//
//void Stream::add_filter (const StreamFilterSP& filter) {
//    assert(filter);
//    auto it = filters_.begin();
//    auto pos = it;
//    bool found = false;
//    while (it != filters_.end()) {
//        if ((*it)->type() == filter->type()) {
//            *it = filter;
//            return;
//        }
//        if ((*it)->priority() > filter->priority() && !found) {
//            pos = it;
//            found = true;
//        }
//        it++;
//    }
//    if (found) filters_.insert(pos, filter);
//    else filters_.push_back(filter);
//}
//
//StreamFilterSP Stream::get_filter (const void* type) const {
//    for (const auto& f : filters_) if (f->type() == type) return f;
//    return {};
//}
//
//void Stream::use_ssl (SSL_CTX* context) { add_filter(new ssl::SSLFilter(this, context)); }
//
//void Stream::use_ssl (const SSL_METHOD* method) {
//    ssl::SSLFilterSP f = new ssl::SSLFilter(this, method);
//    if (listening() && f->is_client()) throw Error("Using ssl without certificate on listening stream");
//    add_filter(f);
//}
//
//bool Stream::is_secure () const { return get_filter(ssl::SSLFilter::TYPE); }
//
//SSL* Stream::get_ssl () const {
//    auto filter = get_filter<ssl::SSLFilter>();
//    return filter ? filter->get_ssl() : nullptr;
//}
//
//void Stream::_close () {
//    Handle::_close();
//}
//
//void Stream::cancel_connect () {
//    _EDEBUGTHIS("cancel_connect");
//    call_delayed();
//    _close();
//}
//
//void Stream::asyncq_cancel_connect (CommandBase* last_tail) {
//    _EDEBUGTHIS("asyncq_cancel_connect");
//    if (asyncq.tail != last_tail) { // reconnect possible
//        // now ensure that last_tail is still in async queue (reset hasn't been called)
//        bool found = false;
//        for (const CommandBase* cmd = asyncq.head; cmd; cmd = cmd->next) if (cmd == last_tail) {
//            found = true;
//            break;
//        }
//        if (found) {
//            found = false;
//            for (const CommandBase* cmd = last_tail->next; cmd; cmd = cmd->next) {
//                if (cmd->type == CommandBase::Type::CLOSE_REINIT || cmd->type == CommandBase::Type::SHUTDOWN ||
//                    cmd->type == CommandBase::Type::USER_CALLBACK) continue; // allowed before reconnect
//                if (cmd->type != CommandBase::Type::CONNECT || !static_cast<const CommandConnect*>(cmd)->is_reconnect()) break; // no reconnect
//                found = true;
//                panda_log_debug("reconnect found");
//                break;
//            }
//        }
//        if (found) { // reconnect found, move everything below last_tail to the top of async queue
//            asyncq.tail->next = asyncq.head;
//            asyncq.head = last_tail->next;
//            last_tail->next = nullptr;
//            return;
//        }
//    }
//
//    // reconnect not found -> clear write requests, shutdowns, etc till first disconnect
//    while (asyncq.head && asyncq.head->type != CommandBase::Type::CLOSE_DELETE && asyncq.head->type != CommandBase::Type::CLOSE_REINIT && asyncq.head->type != CommandBase::Type::CONNECT) {
//        CommandBase* cmd = asyncq.head;
//        asyncq.head = cmd->next;
//        cmd->cancel();
//        delete cmd;
//    }
//}
//
//void Stream::on_read (string& buf, const CodeError* err) {
//    if (read_event.has_listeners()) read_event(this, buf, err);
//}
//
//void Stream::on_write (const CodeError* err, WriteRequest* req) {
//    write_event(this, err, req);
//}
//
//void Stream::on_shutdown (const CodeError* err, ShutdownRequest* req) {
//    shutdown_event(this, err, req);
//}
//
//void Stream::on_eof () {
//    eof_event(this);
//}
