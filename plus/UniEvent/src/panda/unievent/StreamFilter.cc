#include "TCP.h"
#include "Stream.h"
#include "StreamFilter.h"

namespace panda { namespace unievent {

void StreamFilters::connect (ConnectRequest* req) {
    if (size()) front()->connect(req);
    else        dyn_cast<TCP*>(handle)->do_connect(static_cast<TCPConnectRequest*>(req));
}

void StreamFilters::on_connect (const CodeError* err, ConnectRequest* req) {
    if (size()) back()->on_connect(err, req);
    else        handle->do_on_connect(err, req);
}

void StreamFilters::on_connection (StreamSP stream, const CodeError* err) {
    if (size()) back()->on_connection(stream, err);
    else        handle->do_on_connection(stream, err);
}

void StreamFilters::write (WriteRequest* req) {
    if (size()) front()->write(req);
    else        handle->do_write(req);
}

void StreamFilters::on_write (const CodeError* err, WriteRequest* req) {
    if (size()) back()->on_write(err, req);
    else        handle->do_on_write(err, req);
}

void StreamFilters::on_read (string& buf, const CodeError* err) {
    if (size()) back()->on_read(buf, err);
    else        handle->do_on_read(buf, err);
}

void StreamFilters::on_shutdown (const CodeError* err, ShutdownRequest* req) {
    if (size()) back()->on_shutdown(err, req);
    else        handle->do_on_shutdown(err, req);

}

void StreamFilters::on_eof () {
    if (size()) back()->on_eof();
    else        handle->do_on_eof();
}

StreamFilter::StreamFilter (Stream* h, const void* type, double priority) : handle(h), _type(type), _priority(priority) {}

void StreamFilter::set_connecting () {
    handle->set_connecting();
}

void StreamFilter::set_connected (bool success) {
    handle->set_connected(success);
}

void StreamFilter::set_shutdown (bool success) {
    handle->set_shutdown(success);
}

bool StreamFilter::is_secure () {
    return false;
}

CodeError StreamFilter::temp_read_start () {
    return handle->_read_start();
}

void StreamFilter::restore_read_start () {
    if (!handle->wantread()) handle->read_stop();
}

void StreamFilter::connect (ConnectRequest* req) {
    if (next_) next_->connect(req);
    else       dyn_cast<TCP*>(handle)->do_connect(static_cast<TCPConnectRequest*>(req));
}

void StreamFilter::on_connect (const CodeError* err, ConnectRequest* req) {
    if (prev_) prev_->on_connect(err, req);
    else       handle->do_on_connect(err, req);
}

void StreamFilter::on_connection (StreamSP stream, const CodeError* err) {
    if (prev_) prev_->on_connection(stream, err);
    else       handle->do_on_connection(stream, err);
}

void StreamFilter::write (WriteRequest* req) {
    if (next_) next_->write(req);
    else       handle->do_write(req);
}

void StreamFilter::on_write (const CodeError* err, WriteRequest* req) {
    if (prev_) prev_->on_write(err, req);
    else       handle->do_on_write(err, req);
}

void StreamFilter::on_read (string& buf, const CodeError* err) {
    if (prev_) prev_->on_read(buf, err);
    else       handle->do_on_read(buf, err);
}

void StreamFilter::on_shutdown (const CodeError* err, ShutdownRequest* req) {
    if (prev_) prev_->on_shutdown(err, req);
    else       handle->do_on_shutdown(err, req);
}

void StreamFilter::on_eof () {
    if (prev_) prev_->on_eof();
    else       handle->do_on_eof();
}

void StreamFilter::on_reinit () {
    if (prev_) prev_->on_reinit();
}

}} // namespace panda::event
