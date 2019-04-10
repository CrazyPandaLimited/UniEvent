#pragma once
#include "Stream.h"
#include "backend/BackendPipe.h"

namespace panda { namespace unievent {

struct Pipe : virtual Stream {
    Pipe (Loop* loop = Loop::default_loop(), bool ipc = false) : ipc(ipc) {
        _ECTOR();
        _init(loop, loop->impl()->new_pipe(this, ipc));
    }

    Pipe (bool ipc) : Pipe(Loop::default_loop(), ipc) {}

    ~Pipe () { _EDTOR(); }

    const HandleType& type () const override;

//    virtual void open              (file_t file);
    virtual void bind    (string_view name);
    virtual void connect (const PipeConnectRequestSP& req);
            void connect (const string& name, connect_fn callback = nullptr); // BOTTOM-INLINE

//    virtual void pending_instances (int count);
//
//    size_t getsocknamelen () const {
//        return getsockname(nullptr, 0);
//    }
//
//    size_t getpeernamelen () const {
//        return getpeername(nullptr, 0);
//    }
//
//    size_t getsockname (char* name, size_t namelen) const {
//        int err = uv_pipe_getsockname(&uvh, name, &namelen);
//        if (err && err != UV_ENOBUFS) throw CodeError(err);
//        return namelen;
//    }
//
//    size_t getpeername (char* name, size_t namelen) const {
//        int err = uv_pipe_getpeername(&uvh, name, &namelen);
//        if (err && err != UV_ENOBUFS) throw CodeError(err);
//        return namelen;
//    }
//
//    panda::string getsockname () const {
//        panda::string str(getsocknamelen());
//        getsockname(str.buf(), (size_t)-1);
//        return str;
//    }
//
//    panda::string getpeername () const {
//        panda::string str(getpeernamelen());
//        getpeername(str.buf(), (size_t)-1);
//        return str;
//    }

    static const HandleType TYPE;

protected:
//    void on_handle_reinit () override;

    StreamSP create_connection () override;

private:
    friend PipeConnectRequest;

    bool ipc;

    backend::BackendPipe* impl () const { return static_cast<backend::BackendPipe*>(_impl); }
};


struct PipeConnectRequest : ConnectRequest, panda::lib::AllocatedObject<PipeConnectRequest> {
    string name;

    PipeConnectRequest (const string& name, Stream::connect_fn callback = nullptr)
        : ConnectRequest(callback), name(name) {}

private:
    friend Pipe;
    Pipe* handle;

    void set (Pipe* h) {
        handle = h;
        ConnectRequest::set(h);
    }

    void exec () override;
};


inline void Pipe::connect (const string& name, connect_fn callback) {
    connect(new PipeConnectRequest(name, callback));
}


}}
