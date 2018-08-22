#include <panda/unievent/Pipe.h>
using namespace panda::unievent;

void Pipe::open (file_t file) {
    int uverr = uv_pipe_open(&uvh, file);
    if (uverr) throw PipeError(uverr);
    if (flags & SF_WANTREAD) read_start();
}

void Pipe::bind (string_view name) {
    PEXS_NULL_TERMINATE(name, name_str);
    int err = uv_pipe_bind(&uvh, name_str);
    if (err) throw PipeError(err);
}

void Pipe::connect (const string& name, ConnectRequest* req) {
    if (!req) req = new ConnectRequest();
    _pex_(req)->handle = _pex_(this);

    if (async_locked()) {
        asyncq_push(new CommandConnectPipe(this, name, req));
        return;
    }
    
    req->retain();
    if (connecting()) {
        req->release();
        throw PipeError(UV_EALREADY);
    }

    PEXS_NULL_TERMINATE(name, name_str);
    uv_pipe_connect(_pex_(req), &uvh, name_str, Stream::uvx_on_connect);

    flags |= SF_CONNECTING;
    async_lock();
    retain();
}

void Pipe::pending_instances (int count) {
    uv_pipe_pending_instances(&uvh, count);
}

void Pipe::on_handle_reinit () {
    int err = uv_pipe_init(uvh.loop, &uvh, ipc);
    if (err) throw PipeError(err);
    Stream::on_handle_reinit();
}
