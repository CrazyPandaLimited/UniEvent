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
    /*INL*/ void connect (const string& name, connect_fn callback = nullptr);

//    virtual void pending_instances (int count);

    panda::string sockname () const { return impl()->sockname(); }
    panda::string peername () const { return impl()->peername(); }

    static const HandleType TYPE;

protected:
//    void on_handle_reinit () override;

    StreamSP create_connection () override;

private:
    friend PipeConnectRequest;

    bool ipc;

    backend::BackendPipe* impl () const { return static_cast<backend::BackendPipe*>(Handle::impl()); }
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
