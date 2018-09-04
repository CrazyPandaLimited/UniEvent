#include <panda/unievent/Command.h>
#include <panda/unievent/TCP.h>
#include <panda/unievent/Pipe.h>

using namespace panda::unievent;

void CommandCloseDelete::cancel () {}
void CommandCloseDelete::run    () { handle->close_delete(); }


void CommandCloseReinit::cancel () {}
void CommandCloseReinit::run    () { handle->close_reinit(true); }


void CommandConnect::cancel () {
    tcp->retain(); //
    req->retain(); // because no 'connect' ever called
    CodeError err(ERRNO_ECANCELED);
    tcp->call_on_connect(&err, req, false);
}

CommandConnect::~CommandConnect () {
    req->release();
}

void CommandResolveHost::cancel() {}

void CommandResolveHost::run() {
    stream_->resolve(host_, service_, has_hints_ ? &hints_ : nullptr, callback_, use_cached_resolver_);
}

void CommandConnectPipe::run () {
    pipe->connect(_name, req);
}

void CommandConnectPipe::cancel () {
    pipe->retain(); //
    req->retain(); // because no 'connect' ever called
    CodeError err(ERRNO_ECANCELED);
    pipe->call_on_connect(&err, req, false);
}

CommandConnectPipe::~CommandConnectPipe () {
    req->release();
}

void CommandConnectSockaddr::run () {
    tcp->connect((sockaddr*)&sa4, req);
}

void CommandConnectHost::run () {
    tcp->connect(host, service, has_hints ? &hints : nullptr, req, use_cached_resolver);
}

void CommandWrite::run () {
    stream->write(req);
}

void CommandWrite::cancel () {
    stream->retain(); // because no 'write' ever called
    req->retain();    //
    CodeError err(ERRNO_ECANCELED);
    stream->call_on_write(&err, req);
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
    CodeError err(ERRNO_ECANCELED);
    stream->call_on_shutdown(&err, req, false);
}

CommandShutdown::~CommandShutdown () {
    req->release();
}

void CommandCallback::cancel () {}
void CommandCallback::run    () { cb(handle); }
