#include "StreamFilter.h"
#include "Stream.h"
//#include "TCP.h"

using namespace panda::unievent;

StreamFilter::StreamFilter (Stream* h, const void* type, double priority) : handle(h), _type(type), _priority(priority) {}

//void StreamFilter::set_connecting () {
//    handle->set_connecting();
//}
//
//void StreamFilter::set_connected (bool success) {
//    handle->set_connected(success);
//}
//
//void StreamFilter::set_shutdown (bool success) {
//    handle->set_shutdown(success);
//}

bool StreamFilter::is_secure () {
    return false;
}

//CodeError StreamFilter::temp_read_start () {
//    return handle->_read_start();
//}
//
//void StreamFilter::restore_read_start () {
//    if (!handle->wantread()) handle->read_stop();
//}

void StreamFilter::handle_connection (const StreamSP& client, const CodeError* err) {
    invoke(prev, &StreamFilter::handle_connection, &Stream::finalize_handle_connection, client, err);
}

void StreamFilter::handle_connect (const CodeError* err, const ConnectRequestSP& req) {
    invoke(prev, &StreamFilter::handle_connect, &Stream::finalize_handle_connect, err, req);
}

void StreamFilter::handle_read (string& buf, const CodeError* err) {
    invoke(prev, &StreamFilter::handle_read, &Stream::finalize_handle_read, buf, err);
}

void StreamFilter::handle_eof () {
    invoke(prev, &StreamFilter::handle_eof, &Stream::finalize_handle_eof);
}

void StreamFilter::handle_shutdown (const CodeError* err, const ShutdownRequestSP& req) {
    invoke(prev, &StreamFilter::handle_shutdown, &Stream::finalize_handle_shutdown, err, req);
}

//void StreamFilter::connect (ConnectRequest* req) {
//    if (next_) next_->connect(req);
//    else       dyn_cast<TCP*>(handle)->do_connect(static_cast<TCPConnectRequest*>(req));
//}
//
//void StreamFilter::write (WriteRequest* req) {
//    if (next_) next_->write(req);
//    else       handle->do_write(req);
//}
//
//void StreamFilter::on_write (const CodeError* err, WriteRequest* req) {
//    if (prev_) prev_->on_write(err, req);
//    else       handle->do_on_write(err, req);
//}
//
//
//void StreamFilter::on_reinit () {
//    if (prev_) prev_->on_reinit();
//}

void StreamFilter::reset () {
    next->reset();
}
