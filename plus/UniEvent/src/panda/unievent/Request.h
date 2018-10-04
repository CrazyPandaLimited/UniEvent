#pragma once
#include <functional>
#include <panda/refcnt.h>
#include <panda/CallbackDispatcher.h>
#include <panda/unievent/inc.h>
#include <panda/lib/memory.h>
#include <panda/unievent/Error.h>
#include <panda/unievent/Debug.h>
#include <panda/log.h>

namespace panda { namespace unievent {

using panda::lib::AllocatedObject;
class Handle;
class UDP;
class Stream;
class Timer;

class Request : public virtual Refcnt {
public:
    Request () : uvrp(nullptr) {}

protected:
    void _init (void* reqptr) {
        uvrp = static_cast<uv_req_t*>(reqptr);
        uvrp->data = this;
    }

    uv_req_t* uvrp;
};

class CancelableRequest : public Request {
public:
    CancelableRequest () : canceled_(false) {}
    
    bool canceled () const { return canceled_; }
    
    virtual void cancel() {
        _EDEBUGTHIS("cancel");
        if (!canceled_) {
            canceled_ = true;
            // if uv_cancel is succeeded callback will be called with ECANCELED
            // otherwise it is completed or started execution and it is impossible to stop it 
            uv_cancel(uvrp);
        }
    }

protected:
    bool canceled_;
};

class ConnectRequest : public Request, public AllocatedObject<ConnectRequest, true> {
public:
    using connect_fptr = void(Stream* handle, const CodeError* err, ConnectRequest* req);
    using connect_fn = function<connect_fptr>;

    CallbackDispatcher<connect_fptr> event;
    bool is_reconnect;
    //CodeError error;

    ConnectRequest (connect_fn callback = {}, bool is_reconnect = false) : is_reconnect(is_reconnect), timer_(nullptr) {
        _EDEBUGTHIS("callback %p %d", callback, (bool)callback);
        if (callback) { 
            event.add(callback);
        }
        _init(&uvr);
    }

    virtual ~ConnectRequest();

    void set_timer(Timer* timer); 

    void release_timer();

    Handle* handle () { return static_cast<Handle*>(uvr.handle->data); }
    friend uv_connect_t* _pex_ (ConnectRequest*);

private:
    uv_connect_t uvr;
    Timer* timer_;
};

using ConnectRequestSP = iptr<ConnectRequest>;


class ShutdownRequest : public Request, public AllocatedObject<ShutdownRequest, true> {
public:
    using shutdown_fptr = void(Stream* handle, const CodeError* err, ShutdownRequest* req);
    using shutdown_fn = function<shutdown_fptr>;

    CallbackDispatcher<shutdown_fptr> event;

    ShutdownRequest (shutdown_fn callback = {}) {
        if (callback) { 
            event.add(callback);
        }
        _init(&uvr);
    }

    Handle* handle () { return static_cast<Handle*>(uvr.handle->data); }
    friend uv_shutdown_t* _pex_ (ShutdownRequest*);
private:
    uv_shutdown_t uvr;
};


class BufferRequest : public Request {
    static const uint32_t SMALL_BUF_CNT = 4;
public:
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


class WriteRequest : public BufferRequest, public AllocatedObject<WriteRequest, true> {
    uv_write_t uvr;
public:
    using write_fptr = void(Stream* handle, const CodeError* err, WriteRequest* req);
    using write_fn = function<write_fptr>;

    CallbackDispatcher<write_fptr> event;

    WriteRequest (write_fn callback = {}) {
        if(callback) {
            event.add(callback);
        }
        _init(&uvr);
        _ECTOR();
    }

    WriteRequest (const string& data, write_fn callback = {}) : BufferRequest(data)  {
        if (callback) {
            event.add(callback);
        }
        _init(&uvr);
        _ECTOR();
    }

    template <class It>
    WriteRequest (It begin, It end, write_fn callback = {}) : BufferRequest(begin, end) {
        if(callback) {
            event.add(callback);
        }
        _init(&uvr);
        _ECTOR();
    }

    Handle* handle () const { return static_cast<Handle*>(uvr.handle->data); }

    virtual ~WriteRequest () {
        _EDTOR();
    }
    friend uv_write_t* _pex_ (WriteRequest*);
    friend class Stream;
};


class SendRequest : public BufferRequest, public AllocatedObject<SendRequest, true> {
    uv_udp_send_t uvr;
public:
    using send_fptr = void(UDP* handle, const CodeError* err, SendRequest* req);
    using send_fn = function<send_fptr>;

    CallbackDispatcher<send_fptr> event;

    SendRequest (send_fn callback = {}) : BufferRequest() {
        if(callback) {
            event.add(callback);
        }
        _init(&uvr);
    }

    SendRequest (const string& data, send_fn callback = {}) : BufferRequest(data) {
        if(callback) {
            event.add(callback);
        }
        _init(&uvr);
    }

    template <class It>
    SendRequest (It begin, It end, send_fn callback = {}) : BufferRequest(begin, end) {
        if(callback) {
            event.add(callback);
        }
        _init(&uvr);
    }

    Handle* handle () const { return static_cast<Handle*>(uvr.handle->data); }

    virtual ~SendRequest () {}
    friend uv_udp_send_t* _pex_ (SendRequest*);
};

inline uv_connect_t*  _pex_(ConnectRequest* req) { return &req->uvr; }
inline uv_shutdown_t* _pex_(ShutdownRequest* req) { return &req->uvr; }
inline uv_write_t*    _pex_(WriteRequest* req) { return &req->uvr; }
inline uv_udp_send_t* _pex_(SendRequest* req) { return &req->uvr; }
}}
