#include "Pipe.h"
#include "util.h"
using namespace panda::unievent;

const HandleType Pipe::TYPE("pipe");

const HandleType& Pipe::type () const {
    return TYPE;
}

backend::BackendHandle* Pipe::new_impl () {
    return loop()->impl()->new_pipe(this, _ipc);
}

void Pipe::open (file_t file, Ownership ownership) {
    if (ownership == Ownership::SHARE) file = file_dup(file);
    impl()->open(file);
    if (peername()) {
        auto err = set_connect_result(true);
        if (err) throw err;
    }
}

void Pipe::bind (string_view name) {
    impl()->bind(name);
}

StreamSP Pipe::create_connection () {
    return new Pipe(loop(), _ipc);
}

void Pipe::connect (const PipeConnectRequestSP& req) {
    req->set(this);
    queue.push(req);
}

void PipeConnectRequest::exec () {
    ConnectRequest::exec();
    auto err = handle->impl()->connect(name, impl());
    if (err) return delay([=]{ cancel(err); });
}

void Pipe::pending_instances (int count) {
    impl()->pending_instances(count);
}
