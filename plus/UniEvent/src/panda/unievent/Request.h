#pragma once
#include "inc.h"
#include "BackendHandle.h"
#include <vector>
#include <panda/string.h>
#include <panda/refcnt.h>
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

struct Request;
using RequestSP = iptr<Request>;

struct Request : panda::lib::IntrusiveChainNode<RequestSP>, Refcnt, protected backend::IRequestListener {
    template <class Func>
    void delay (Func&& f) {
        if (_delay_id) _handle->loop()->cancel_delay(_delay_id);
        _delay_id = _handle->loop()->delay(f);
    }

protected:
    using RequestImpl = backend::RequestImpl;
    friend struct Queue; friend StreamFilter;

    RequestImpl* _impl;
    Request*     parent;
    RequestSP    subreq;

    Request () : _impl(), parent(), _delay_id(0) {}

    void set (BackendHandle* h) {
        _handle = h;
    }

    virtual void exec () = 0;

    /* this is private API, as there is no way of stopping request inside backend in general case. usually called during reset()
       If called separately by user, will only do "visible" cancellation (user callback being called with canceled status),
       but backend will continue to run the request and the next request will only be started afterwards */
    virtual void cancel (const CodeError& err = std::errc::operation_canceled) {
        if (subreq) return subreq->cancel(err);
        handle_event(err);
    }

    // detach from backend. Backend won't call the callback when request is completed (if it wasn't completed already)
    void finish_exec () {
        if (_delay_id) {
            _handle->loop()->cancel_delay(_delay_id);
            _delay_id = 0;
        }
        if (_impl) {
            _impl->destroy();
            _impl = nullptr;
        }
    }

    ~Request () {
        assert(!_impl);
    }

private:
    BackendHandleSP _handle;
    uint64_t        _delay_id;
};

}}
