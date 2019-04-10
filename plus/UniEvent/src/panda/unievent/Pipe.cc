#include "Pipe.h"
using namespace panda::unievent;

const HandleType Pipe::TYPE("pipe");

const HandleType& Pipe::type () const {
    return TYPE;
}

//void Pipe::open (file_t file) {
//    int uverr = uv_pipe_open(&uvh, file);
//    if (uverr) throw CodeError(uverr);
//    if (flags & SF_WANTREAD) read_start();
//}

void Pipe::bind (string_view name) {
    impl()->bind(name);
}

void Pipe::connect (const PipeConnectRequestSP& req) {
    req->set(this);
    queue.push(req);
}

void PipeConnectRequest::exec () {
    ConnectRequest::exec();
    auto err = handle->impl()->connect(name, impl());
    if (err) return delay([=]{ handle_connect(err); });
}

//void Pipe::pending_instances (int count) {
//    uv_pipe_pending_instances(&uvh, count);
//}
//
//void Pipe::on_handle_reinit () {
//    int err = uv_pipe_init(uvh.loop, &uvh, ipc);
//    if (err) throw CodeError(err);
//    Stream::on_handle_reinit();
//}

StreamSP Pipe::create_connection () {
    return new Pipe(loop(), ipc);
}
