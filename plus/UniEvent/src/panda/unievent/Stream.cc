#include "Stream.h"
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

// ===================== CONNECTION ===============================
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
    if (!err) {
        client->set_connecting();
        client->set_established();
    }
    invoke(_filters.back(), &StreamFilter::handle_connection, &Stream::finalize_handle_connection, client, err);
}

void Stream::finalize_handle_connection (const StreamSP& client, const CodeError* err) {
    _EDEBUGTHIS("err: %d, client: %p", err ? err->code().value() : 0, client.get());
    err = client->set_connected(!err);
    on_connection(client, err);
}

void Stream::on_connection (const StreamSP& client, const CodeError* err) {
    connection_event(this, client, err);
}

// ===================== CONNECT ===============================
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

void ConnectRequest::cancel () {
    handle_connect(CodeError(std::errc::operation_canceled));
}

void ConnectRequest::handle_connect (const CodeError* err) {
    _EDEBUGTHIS();
    if (!err) handle->set_established();
    handle->invoke(handle->_filters.back(), &StreamFilter::handle_connect, &Stream::finalize_handle_connect, err, this);
}

void Stream::finalize_handle_connect (const CodeError* err, const ConnectRequestSP& req) {
    _EDEBUGTHIS("err: %d, request: %p", err ? err->code().value() : 0, req.get());

    req->timer = nullptr;
    err = set_connected(!err);

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

void Stream::on_connect (const CodeError* err, const ConnectRequestSP& req) {
    connect_event(this, err, req);
}

// ===================== READ ===============================
CodeError Stream::_read_start () {
    if (reading() || !established()) return CodeError();
    auto err = impl()->read_start();
    if (!err) set_reading(true);
    return err;
}

void Stream::read_stop () {
    set_wantread(false);
    if (!reading()) return;
    impl()->read_stop();
    set_reading(false);
}

void Stream::handle_read (string& buf, const CodeError* err) {
    invoke(_filters.back(), &StreamFilter::handle_read, &Stream::finalize_handle_read, buf, err);
}

void Stream::on_read (string& buf, const CodeError* err) {
    read_event(this, buf, err);
}

// ===================== WRITE ===============================
void Stream::write (const WriteRequestSP& req) {
    _EDEBUGTHIS("req: %p", req.get());
    req->set(this);
    queue.push(req);
}

void WriteRequest::exec () {
    _EDEBUGTHIS();
    handle->invoke(handle->_filters.front(), &StreamFilter::write, &Stream::finalize_write, this);
}

void Stream::finalize_write (const WriteRequestSP& req) {
    _EDEBUGTHIS();
    auto err = impl()->write(req->bufs, req->impl());
    if (err) return req->delay([=]{ req->handle_write(err); });
}

void WriteRequest::cancel () {
    handle_write(CodeError(std::errc::operation_canceled));
}

void WriteRequest::handle_write (const CodeError* err) {
    _EDEBUGTHIS();
    handle->invoke(handle->_filters.back(), &StreamFilter::handle_write, &Stream::finalize_handle_write, err, this);
}

void Stream::finalize_handle_write (const CodeError* err, const WriteRequestSP& req) {
    _EDEBUGTHIS("err: %d, request: %p", err ? err->code().value() : 0, req.get());
    queue.done(req, [&]{
        req->event(this, err, req);
        on_write(err, req);
    });
}

void Stream::on_write (const CodeError* err, const WriteRequestSP& req) {
    write_event(this, err, req);
}

// ===================== EOF ===============================
void Stream::handle_eof () {
    invoke(_filters.back(), &StreamFilter::handle_eof, &Stream::finalize_handle_eof);
}

void Stream::on_eof () {
    eof_event(this);
}

// ===================== SHUTDOWN ===============================
void Stream::shutdown (const ShutdownRequestSP& req) {
    _EDEBUGTHIS("req: %p", req.get());
    req->set(this);
    queue.push(req);
}

void ShutdownRequest::exec () {
    _EDEBUGTHIS();
    handle->set_shutting();
    auto err = handle->impl()->shutdown(impl());
    if (err) return delay([=]{ handle_shutdown(err); });
}

void ShutdownRequest::cancel () {
    handle_shutdown(CodeError(std::errc::operation_canceled));
}

void ShutdownRequest::handle_shutdown (const CodeError* err) {
    _EDEBUGTHIS();
    handle->invoke(handle->_filters.back(), &StreamFilter::handle_shutdown, &Stream::finalize_handle_shutdown, err, this);
}

void Stream::finalize_handle_shutdown (const CodeError* err, const ShutdownRequestSP& req) {
    _EDEBUGTHIS("err: %d, request: %p", err ? err->code().value() : 0, req.get());
    set_shutdown(!err);
    queue.done(req, [&]{
        req->event(this, err, req);
        on_shutdown(err, req);
    });
}

void Stream::on_shutdown (const CodeError* err, const ShutdownRequestSP& req) {
    shutdown_event(this, err, req);
}

//void Stream::disconnect () { close_reinit(asyncq_empty() && connecting() ? false : true); }

void Stream::reset () {
    queue.cancel([&]{ do_reset(); });
}

void Stream::do_reset () {
    Handle::reset();
    if (_filters.size()) _filters.front()->reset();
    flags &= DONTREAD; // clear flags except DONTREAD
}

void Stream::clear () {
    queue.cancel([&]{
        Handle::clear();
        flags = 0;
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
