#include "StreamFilter.h"
#include "Stream.h"
#include "Tcp.h"

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

CodeError StreamFilter::read_start () {
    return handle->_read_start();
}

void StreamFilter::read_stop () {
    if (!handle->wantread()) handle->read_stop();
}

void StreamFilter::subreq_tcp_connect (const RequestSP& parent, const TcpConnectRequestSP& req) {
    parent->subreq = req;
    req->parent = parent;
    req->set(panda::dyn_cast<Tcp*>(handle)); // TODO: find a better way
    NextFilter::tcp_connect(req);
}

void StreamFilter::subreq_write (const RequestSP& parent, const WriteRequestSP& req) {
    parent->subreq = req;
    req->parent = parent;
    req->set(handle);
    NextFilter::write(req);
}

void StreamFilter::subreq_done (const RequestSP& req) {
    assert(!req->subreq);
    req->finish_exec();
    req->parent->subreq = nullptr;
    req->parent = nullptr;
}

void StreamFilter::handle_connection (const StreamSP& client, const CodeError& err, const AcceptRequestSP& req) {
    invoke(prev, &StreamFilter::handle_connection, &Stream::finalize_handle_connection, client, err, req);
}

void StreamFilter::tcp_connect (const TcpConnectRequestSP& req) {
    if (next) next->tcp_connect(req);
    else      req->finalize_connect();
}

void StreamFilter::handle_connect (const CodeError& err, const ConnectRequestSP& req) {
    invoke(prev, &StreamFilter::handle_connect, &Stream::finalize_handle_connect, err, req);
}

void StreamFilter::handle_read (string& buf, const CodeError& err) {
    invoke(prev, &StreamFilter::handle_read, &Stream::finalize_handle_read, buf, err);
}

void StreamFilter::write (const WriteRequestSP& req) {
    invoke(next, &StreamFilter::write, &Stream::finalize_write, req);
}

void StreamFilter::handle_write (const CodeError& err, const WriteRequestSP& req) {
    invoke(prev, &StreamFilter::handle_write, &Stream::finalize_handle_write, err, req);
}

void StreamFilter::handle_eof () {
    invoke(prev, &StreamFilter::handle_eof, &Stream::finalize_handle_eof);
}

void StreamFilter::handle_shutdown (const CodeError& err, const ShutdownRequestSP& req) {
    invoke(prev, &StreamFilter::handle_shutdown, &Stream::finalize_handle_shutdown, err, req);
}
