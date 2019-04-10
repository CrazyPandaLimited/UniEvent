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
protected:
    using BackendRequest = backend::BackendRequest;
    friend struct Queue;

    BackendRequest* _impl;

    Request () : _impl(), _delay_id(0) {}

    void set (Handle* h, BackendRequest* impl) {
        assert(!_impl);
        _handle = h;
        _impl   = impl;
    }

    template <class Func>
    void delay (Func&& f) {
        if (_delay_id) _handle->loop()->cancel_delay(_delay_id);
        _delay_id = _handle->loop()->delay(f);
    }

    virtual void exec () = 0;

    /* this is private API, as there is no way of stopping request inside backend in general case. usually called during reset()
       If called separately by user, will only do "visible" cancellation (user callback being called with canceled status),
       but backend will continue to run the request and the next request will only be started afterwards */
    virtual void cancel () {
        if (_delay_id) {
            _handle->loop()->cancel_delay(_delay_id);
            _delay_id = 0;
        }
        detach();
    }

    // detach from backend. Backend won't call the callback then request is completed
    void detach () {
        _impl->destroy();
        _impl = nullptr;
    }

    ~Request () {
        assert(!_impl);
        if (_impl) _impl->destroy();
    }

private:
    HandleSP _handle;
    uint64_t _delay_id;

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
//
//struct ShutdownRequest : Request, AllocatedObject<ShutdownRequest, true> {
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
