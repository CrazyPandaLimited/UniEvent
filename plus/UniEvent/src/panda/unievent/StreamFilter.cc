#include "TCP.h"
#include "Stream.h"
#include "StreamFilter.h"

namespace panda { namespace unievent {

const char* StreamFilter::TYPE = "EMPTY";

StreamFilter::~StreamFilter() {}

StreamFilter::StreamFilter(Stream* h, const char* type = TYPE) : handle(h), type_(type) {}

void StreamFilter::set_connecting() { handle->set_connecting(); }

void StreamFilter::set_connected(bool success) { handle->set_connected(success); }

void StreamFilter::set_shutdown(bool success) { handle->set_shutdown(success); }

bool StreamFilter::is_secure() { return false; }

CodeError StreamFilter::temp_read_start() { return handle->_read_start(); }

void StreamFilter::restore_read_start() {
    if (!handle->wantread())
        handle->read_stop();
}

const char* BackStreamFilter::TYPE = "BACK";

BackStreamFilter::~BackStreamFilter() {}

BackStreamFilter::BackStreamFilter(Stream* stream) : StreamFilter(stream, TYPE) {}

void BackStreamFilter::connect(ConnectRequest* request) {
    _EDEBUGTHIS("connect");
    dyn_cast<TCP*>(handle)->connect_internal(static_cast<TCPConnectRequest*>(request));
    NextFilter::connect(request);
}

void BackStreamFilter::on_connect(const CodeError* err, ConnectRequest* request) {
    _EDEBUGTHIS("on_connect, err: %d", err ? err->code() : 0);
    NextFilter::on_connect(err, request);
}

void BackStreamFilter::on_connection(StreamSP stream, const CodeError* err) {
    _EDEBUGTHIS("on_connection, err: %d, stream: %p", err ? err->code() : 0, stream);
    NextFilter::on_connection(stream, err);
}

void BackStreamFilter::on_read(string& buf, const CodeError* err) {
    _EDEBUGTHIS("on_read, err: %d", err ? err->code() : 0);
    NextFilter::on_read(buf, err);
}

void BackStreamFilter::write(WriteRequest* request) {
    _EDEBUGTHIS("write, handle: %p, request: %p", handle, request);
    handle->do_write(request);
    NextFilter::write(request);
}

void BackStreamFilter::on_write(const CodeError* err, WriteRequest* request) {
    _EDEBUGTHIS("on_write, err: %d, stream: %p", err ? err->code() : 0, handle);
    NextFilter::on_write(err, request);
}

void BackStreamFilter::on_shutdown(const CodeError* err, ShutdownRequest* shutdown_request) {
    _EDEBUGTHIS("on_shutdown");
    NextFilter::on_shutdown(err, shutdown_request);
}

void BackStreamFilter::on_eof() {
    _EDEBUGTHIS("on_eof");
    NextFilter::on_eof();
}

void BackStreamFilter::on_reinit() {
    _EDEBUGTHIS("on_reinit");
    bool wanted_read = handle->wantread();
    handle->Handle::on_handle_reinit();
    if (wanted_read) handle->set_wantread();
    NextFilter::on_reinit();
}

const char* FrontStreamFilter::TYPE = "FRONT";

FrontStreamFilter::~FrontStreamFilter() {}

FrontStreamFilter::FrontStreamFilter(Stream* stream) : StreamFilter(stream, TYPE) {}

void FrontStreamFilter::connect(ConnectRequest* request) {
    _EDEBUGTHIS("connect");
    NextFilter::connect(request);
}

void FrontStreamFilter::on_connect(const CodeError* err, ConnectRequest* connect_request) {
    _EDEBUGTHIS("on_connect, err: %d, stream: %p, request: %p", err ? err->code() : 0, handle, connect_request);
    bool unlock = !(err && err->code() == ERRNO_ECANCELED);
    handle->set_connected(!err);
    connect_request->release_timer();
    if (handle->asyncq_empty()) {
        if (unlock)
            handle->async_unlock_noresume();
        {
            auto guard = handle->lock_in_callback();
            connect_request->event(handle, err, connect_request);
            handle->on_connect(err, connect_request);
        }
        connect_request->release();
    } else {
        CommandBase* last_tail = handle->asyncq.tail;
        {
            auto guard = handle->lock_in_callback();
            connect_request->event(handle, err, connect_request);
            handle->on_connect(err, connect_request);
        }
        connect_request->release();
        if (err) {
            // remove writes, shutdowns, etc - till first disconnect - only if no reconnect
            handle->asyncq_cancel_connect(last_tail); 
        }
        if (unlock) {
            handle->async_unlock();
        }
    }
    handle->release();
    NextFilter::on_connect(err, connect_request);
}

void FrontStreamFilter::on_connection(StreamSP stream, const CodeError* err) {
    _EDEBUGTHIS("on_connection, err: %d, stream: %p", err ? err->code() : 0, stream);
    stream->set_connected(!err);
    {
        auto guard = stream->lock_in_callback();
        handle->on_connection(stream, err);
    }
    stream->release();
    NextFilter::on_connection(stream, err);
}

void FrontStreamFilter::on_read(string& buf, const CodeError* err) {
    _EDEBUGTHIS("on_read, err: %d", err ? err->code() : 0);
    handle->on_read(buf, err);
    NextFilter::on_read(buf, err);
}

void FrontStreamFilter::write(WriteRequest* request) {
    _EDEBUGTHIS("write, handle: %p, request: %p", handle, request);
    NextFilter::write(request);
}

void FrontStreamFilter::on_write(const CodeError* err, WriteRequest* write_request) {
    _EDEBUGTHIS("on_write, err: %d, handle: %p, request: %p", err ? err->code() : 0, handle, write_request);
    {
        auto guard = handle->lock_in_callback();
        write_request->event(handle, err, write_request);
        handle->on_write(err, write_request);
    }
    write_request->release();
    handle->release();
    NextFilter::on_write(err, write_request);
}

void FrontStreamFilter::on_shutdown(const CodeError* err, ShutdownRequest* shutdown_request) {
    _EDEBUGTHIS("on_shutdown");
    bool unlock = !(err && err->code() == ERRNO_ECANCELED);
    handle->set_shutdown(!err);
    {
        auto guard = handle->lock_in_callback();
        shutdown_request->event(handle, err, shutdown_request);
        handle->on_shutdown(err, shutdown_request);
    }
    shutdown_request->release();
    if (unlock) {
        handle->async_unlock();
    }
    handle->release();
    NextFilter::on_shutdown(err, shutdown_request);
}

void FrontStreamFilter::on_eof() {
    _EDEBUGTHIS("on_eof");
    handle->set_connected(false);
    handle->on_eof();
    NextFilter::on_eof();
}

void FrontStreamFilter::on_reinit() {
    _EDEBUGTHIS("on_reinit");
    NextFilter::on_reinit();
}

}} // namespace panda::event
