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

bool StreamFilter::is_secure () {
    return false;
}

CodeError StreamFilter::priority_read_start () {
    return handle->_read_start();
}

void StreamFilter::priority_read_stop () {
    if (!handle->wantread()) handle->read_stop();
}

void StreamFilter::subreq_tcp_connect (const RequestSP& parent, const TcpConnectRequestSP& subreq) {
    parent->subreq = subreq;
    subreq->parent = parent;
    NextFilter::tcp_connect(subreq);
}

void StreamFilter::subreq_write (const RequestSP& parent, const WriteRequestSP& subreq) {
    parent->subreq = subreq;
    subreq->parent = parent;
    NextFilter::write(subreq);
}


void StreamFilter::handle_connection (const StreamSP& client, const CodeError& err) {
    invoke(prev, &StreamFilter::handle_connection, &Stream::finalize_handle_connection, client, err);
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

void StreamFilter::reset () {
    next->reset();
}
