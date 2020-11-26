#include "Pipe.h"
#include "util.h"

namespace panda { namespace unievent {

const HandleType Pipe::TYPE("pipe");

const HandleType& Pipe::type () const {
    return TYPE;
}

backend::HandleImpl* Pipe::new_impl () {
    return loop()->impl()->new_pipe(this, _ipc);
}

excepted<void, ErrorCode> Pipe::open (fd_t file, Ownership ownership, bool connected) {
    if (ownership == Ownership::SHARE) file = file_dup(file);

    auto error = impl()->open(file);
    if (error) return make_unexpected(ErrorCode(error));

    if (connected || peername()) {
        error = set_connect_result(true);
    }
    return make_excepted(error);
}

excepted<void, ErrorCode> Pipe::bind (string_view name) {
    return make_excepted(impl()->bind(name));
}

StreamSP Pipe::create_connection () {
    return new Pipe(loop(), _ipc);
}

PipeConnectRequestSP Pipe::connect (const PipeConnectRequestSP& req) {
    req->set(this);
    queue.push(req);
    return req;
}

void PipeConnectRequest::exec () {
    ConnectRequest::exec();
    if (handle->filters().size()) {
        last_filter = handle->filters().front();
        last_filter->pipe_connect(this);
    }
    else finalize_connect();
}

void PipeConnectRequest::finalize_connect () {
    panda_log_debug("PipeConnectRequest::finalize_connect " << this);
    auto err = handle->impl()->connect(name, impl());
    if (err) return delay([=]{ cancel(err); });
}

void Pipe::pending_instances (int count) {
    impl()->pending_instances(count);
}


}}
