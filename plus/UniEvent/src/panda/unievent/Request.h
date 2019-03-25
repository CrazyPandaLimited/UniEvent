#pragma once
#include "inc.h"
#include "Handle.h"
#include <vector>
#include <panda/string.h>
#include <panda/refcnt.h>
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

struct Request;
using RequestSP = iptr<Request>;

struct Request : panda::lib::IntrusiveChainNode<RequestSP>, Refcnt {
    Request () : _impl(), delay_id(0) {}

protected:
    using BackendRequest = backend::BackendRequest;
    friend struct Queue;

    HandleSP        _handle;
    BackendRequest* _impl;
    uint64_t        delay_id;

    void set (Handle* h, BackendRequest* impl) {
        _handle = h;
        _impl = impl;
    }

    virtual void exec      () = 0;
    virtual void on_cancel () = 0;

    // abort request, calling user callbacks with canceled status, if request is active in backend, backend will not call callback second time
    // this is a private API not intended by use of user, as it won't start processing the next request in queue
    void abort () {
        if (delay_id) {
            _handle->loop()->cancel_delay(delay_id);
            delay_id = 0;
        }
        _impl->destroy();
        _impl = nullptr;
        on_cancel();
    }

    ~Request () {
        if (_impl) _impl->destroy();
    }
};

struct BufferRequest : Request {
    std::vector<string> bufs;

    BufferRequest () {}

    BufferRequest (const string& data) {
        bufs.push_back(data);
    }

    template <class It>
    BufferRequest (It begin, It end) {
        bufs.reserve(end - begin);
        for (; begin != end; ++begin) bufs.push_back(*begin);
    }
};

//struct CancelableRequest : Request {
//    CancelableRequest () : canceled_(false) {}
//
//    bool canceled () const { return canceled_; }
//
//    virtual void cancel() {
//        _EDEBUGTHIS("cancel");
//        if (!canceled_) {
//            canceled_ = true;
//            // if uv_cancel is succeeded callback will be called with ECANCELED
//            // otherwise it is completed or started execution and it is impossible to stop it
//            uv_cancel(uvrp);
//        }
//    }
//
//protected:
//    bool canceled_;
//};
//
//struct ConnectRequest : Request, AllocatedObject<ConnectRequest, true> {
//    using connect_fptr = void(Stream* handle, const CodeError* err, ConnectRequest* req);
//    using connect_fn = function<connect_fptr>;
//
//    CallbackDispatcher<connect_fptr> event;
//    bool is_reconnect;
//
//    ConnectRequest (connect_fn callback = {}, bool is_reconnect = false) : is_reconnect(is_reconnect), timer_(nullptr) {
//        _EDEBUGTHIS("callback %d", (bool)callback);
//        if (callback) {
//            event.add(callback);
//        }
//        _init(&uvr);
//    }
//
//    virtual ~ConnectRequest();
//
//    void set_timer(Timer* timer);
//
//    void release_timer();
//
//    Handle* handle () { return static_cast<Handle*>(uvr.handle->data); }
//    friend uv_connect_t* _pex_ (ConnectRequest*);
//
//private:
//    uv_connect_t uvr;
//    Timer* timer_;
//};
//
//struct ShutdownRequest : Request, AllocatedObject<ShutdownRequest, true> {
//    using shutdown_fptr = void(Stream* handle, const CodeError* err, ShutdownRequest* req);
//    using shutdown_fn = function<shutdown_fptr>;
//
//    CallbackDispatcher<shutdown_fptr> event;
//
//    ShutdownRequest (shutdown_fn callback = {}) {
//        if (callback) {
//            event.add(callback);
//        }
//        _init(&uvr);
//    }
//
//    Handle* handle () { return static_cast<Handle*>(uvr.handle->data); }
//    friend uv_shutdown_t* _pex_ (ShutdownRequest*);
//private:
//    uv_shutdown_t uvr;
//};
//
//struct WriteRequest : BufferRequest, AllocatedObject<WriteRequest, true> {
//    using write_fptr = void(Stream* handle, const CodeError* err, WriteRequest* req);
//    using write_fn = function<write_fptr>;
//
//    CallbackDispatcher<write_fptr> event;
//
//    WriteRequest (write_fn callback = {}) {
//        if(callback) {
//            event.add(callback);
//        }
//        _init(&uvr);
//        _ECTOR();
//    }
//
//    WriteRequest (const string& data, write_fn callback = {}) : BufferRequest(data)  {
//        if (callback) {
//            event.add(callback);
//        }
//        _init(&uvr);
//        _ECTOR();
//    }
//
//    template <class It>
//    WriteRequest (It begin, It end, write_fn callback = {}) : BufferRequest(begin, end) {
//        if(callback) {
//            event.add(callback);
//        }
//        _init(&uvr);
//        _ECTOR();
//    }
//
//    Handle* handle () const { return static_cast<Handle*>(uvr.handle->data); }
//
//    virtual ~WriteRequest () {
//        _EDTOR();
//    }
//    friend uv_write_t* _pex_ (WriteRequest*);
//    friend struct Stream;
//
//private:
//    uv_write_t uvr;
//};

}}
