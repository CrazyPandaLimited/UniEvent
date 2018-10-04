#include <panda/unievent/Command.h>
#include <panda/unievent/TCP.h>
#include <panda/unievent/Pipe.h>

using namespace panda::unievent;

void CommandCloseDelete::cancel () {}
void CommandCloseDelete::run    () { handle->close_delete(); }


void CommandCloseReinit::cancel () {}
void CommandCloseReinit::run    () { handle->close_reinit(true); }

CommandConnect::CommandConnect(TCP* tcp, TCPConnectRequest* tcp_connect_request) : tcp(tcp), tcp_connect_request(tcp_connect_request) {
    type = Type::CONNECT;
    tcp_connect_request->retain();
}

void CommandConnect::cancel() {
    tcp->retain(); //
    tcp_connect_request->retain(); // because no 'connect' ever called
    tcp->get_front_filter()->on_connect(CodeError(ERRNO_ECANCELED), tcp_connect_request);
}

void CommandConnect::run() { tcp->connect(tcp_connect_request); }

bool CommandConnect::is_reconnect() const { return tcp_connect_request->is_reconnect; }

CommandConnect::~CommandConnect() { tcp_connect_request->release(); }

void CommandConnectPipe::run () {
    pipe->connect(_name, req);
}

void CommandConnectPipe::cancel () {
    pipe->retain(); //
    req->retain(); // because no 'connect' ever called
    pipe->get_front_filter()->on_connect(CodeError(ERRNO_ECANCELED), req);
}

CommandConnectPipe::~CommandConnectPipe () {
    req->release();
}

void CommandWrite::run () {
    stream->write(req);
}

void CommandWrite::cancel () {
    stream->retain(); // because no 'write' ever called
    req->retain();    //
    stream->get_front_filter()->on_write(CodeError(ERRNO_ECANCELED), req);
}

CommandWrite::~CommandWrite () {
    req->release();
}

void CommandShutdown::run () {
    stream->shutdown(req);
}

void CommandShutdown::cancel () {
    stream->retain();
    req->retain();
    stream->get_front_filter()->on_shutdown(CodeError(ERRNO_ECANCELED), req);
}

CommandShutdown::~CommandShutdown () {
    req->release();
}

void CommandCallback::cancel () {}
void CommandCallback::run    () { cb(handle); }
