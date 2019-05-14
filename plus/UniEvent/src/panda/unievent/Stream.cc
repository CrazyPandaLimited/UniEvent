#include "Stream.h"
#include "ssl/SslFilter.h"

namespace panda { namespace unievent {

using ssl::SslFilter;

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
    invoke_sync(&StreamFilter::listen);
    if (callback) connection_event.add(callback);
    impl()->listen(backlog);
    set_listening();
}

void Stream::handle_connection (const CodeError& err) {
    _EDEBUG("[%p] err: %d", this, err.code().value());
    if (err) invoke(_filters.back(), &StreamFilter::handle_connection, &Stream::finalize_handle_connection, nullptr, err, nullptr);
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
    // filters may delay handle_connection() and make subrequests
    // creating dummy AcceptRequest follows 2 purposes: holding the only client reference and delaying users requests until handle_connection() is done
    AcceptRequestSP areq;
    if (_filters.size()) {
        areq = new AcceptRequest(client);
        client->queue.push(areq);
    }
    invoke(_filters.back(), &StreamFilter::handle_connection, &Stream::finalize_handle_connection, client, err, areq);
}

void Stream::finalize_handle_connection (const StreamSP& client, const CodeError& err1, const AcceptRequestSP& req) {
    auto err2 = client->set_connect_result(!err1);
    auto& err = err1 ? err1 : err2;
    _EDEBUGTHIS("err: %d, client: %p", err.code().value(), client.get());
    if (req) client->queue.done(req, []{});
    on_connection(client, err);
}

void Stream::on_connection (const StreamSP& client, const CodeError& err) {
    connection_event(this, client, err);
}

// ===================== CONNECT ===============================
void ConnectRequest::exec () {
    _EDEBUGTHIS();
    handle->set_connecting();

    if (timeout) {
        timer = new Timer(handle->loop());
        timer->event.add([this](const TimerSP&){ cancel(std::errc::timed_out); });
        timer->once(timeout);
    }
}

void ConnectRequest::handle_event (const CodeError& err) {
    _EDEBUGTHIS();
    if (!err) handle->set_established();
    handle->invoke(handle->_filters.back(), &StreamFilter::handle_connect, &Stream::finalize_handle_connect, err, this);
}

void Stream::finalize_handle_connect (const CodeError& err1, const ConnectRequestSP& req) {
    auto err2 = set_connect_result(!err1);
    auto& err = err1 ? err1 : err2;
    _EDEBUGTHIS("err: %d, request: %p", err.code().value(), req.get());

    req->timer = nullptr;

    // if we are already canceling queue now, do not start recursive cancel
    if (!err || queue.canceling()) {
        queue.done(req, [&]{
            req->event(this, err, req);
            on_connect(err, req);
        });
    }
    else { // cancel everything till the end of queue, but call connect callback with actual status(err), not with ECANCELED
        queue.cancel([&]{
            queue.done(req, [&]{
                req->event(this, err, req);
                on_connect(err, req);
            });
        }, [&]{
            _reset();
        });
    }
}

void Stream::on_connect (const CodeError& err, const ConnectRequestSP& req) {
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

void Stream::handle_read (string& buf, const CodeError& err) {
    invoke(_filters.back(), &StreamFilter::handle_read, &Stream::finalize_handle_read, buf, err);
}

void Stream::on_read (string& buf, const CodeError& err) {
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
    if (err) return req->delay([=]{ req->cancel(err); });
}

void WriteRequest::handle_event (const CodeError& err) {
    _EDEBUGTHIS();
    handle->invoke(handle->_filters.back(), &StreamFilter::handle_write, &Stream::finalize_handle_write, err, this);
}

void Stream::finalize_handle_write (const CodeError& err, const WriteRequestSP& req) {
    _EDEBUGTHIS("err: %d, request: %p", err.code().value(), req.get());
    queue.done(req, [&]{
        req->event(this, err, req);
        on_write(err, req);
    });
}

void Stream::on_write (const CodeError& err, const WriteRequestSP& req) {
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
    if (err) return delay([=]{ cancel(err); });
}

void ShutdownRequest::handle_event (const CodeError& err) {
    _EDEBUGTHIS();
    handle->invoke(handle->_filters.back(), &StreamFilter::handle_shutdown, &Stream::finalize_handle_shutdown, err, this);
}

void Stream::finalize_handle_shutdown (const CodeError& err, const ShutdownRequestSP& req) {
    _EDEBUGTHIS("err: %d, request: %p", err.code().value(), req.get());
    set_shutdown(!err);
    queue.done(req, [&]{
        req->event(this, err, req);
        on_shutdown(err, req);
    });
}

void Stream::on_shutdown (const CodeError& err, const ShutdownRequestSP& req) {
    shutdown_event(this, err, req);
}

// ===================== DISCONNECT/RESET/CLEAR ===============================
struct DisconnectRequest : StreamRequest {
    DisconnectRequest (Stream* h) { set(h); }

    void exec         ()                                                 override { handle->queue.done(this, [&]{ handle->_reset(); }); }
    void cancel       (const CodeError& = std::errc::operation_canceled) override { handle->queue.done(this, []{}); }
    void handle_event (const CodeError&)                                 override {}
};

void Stream::disconnect () {
    if (!queue.size()) _reset();
    else if (queue.size() == 1 && connecting()) reset();
    else queue.push(new DisconnectRequest(this));
}

void Stream::reset () {
    queue.cancel([&]{ _reset(); });
}

void Stream::_reset () {
    Handle::reset();
    invoke_sync(&StreamFilter::reset);
    flags &= DONTREAD; // clear flags except DONTREAD
}

void Stream::clear () {
    queue.cancel([&]{ _clear(); });
}

void Stream::_clear () {
    Handle::clear();
    invoke_sync(&StreamFilter::reset);
    _filters.clear();
    flags = 0;
    buf_alloc_callback = nullptr;
    connection_factory = nullptr;
    connection_event.remove_all();
    connect_event.remove_all();
    read_event.remove_all();
    write_event.remove_all();
    shutdown_event.remove_all();
    eof_event.remove_all();
}

// ===================== FILTERS ADD/REMOVE ===============================
void Stream::add_filter (const StreamFilterSP& filter) {
    assert(filter);
    auto it = _filters.begin();
    auto pos = it;
    bool found = false;
    while (it != _filters.end()) {
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
    if (found) _filters.insert(pos, filter);
    else _filters.push_back(filter);
}

StreamFilterSP Stream::get_filter (const void* type) const {
    for (const auto& f : _filters) if (f->type() == type) return f;
    return {};
}

void Stream::use_ssl (SSL_CTX* context)         { add_filter(new SslFilter(this, context)); }
void Stream::use_ssl (const SSL_METHOD* method) { add_filter(new SslFilter(this, method)); }

bool Stream::is_secure () const { return get_filter(SslFilter::TYPE); }

SSL* Stream::get_ssl () const {
    auto filter = get_filter<SslFilter>();
    return filter ? filter->get_ssl() : nullptr;
}

}}
