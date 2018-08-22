#pragma once
#include <panda/unievent/Error.h>
#include <panda/unievent/Request.h>
#include <panda/unievent/Stream.h>

namespace panda { namespace unievent {

class StreamFilter : public Refcnt {
public:
    static const char* TYPE;

    const char* type () const { return _type; }

    virtual void accept        (Stream* client);
    virtual void write         (WriteRequest* req);
    virtual void on_connect    (const StreamError& err, ConnectRequest* req);
    virtual void on_write      (const StreamError& err, WriteRequest* req);
    virtual void on_read       (const string& buf, const StreamError& err);
    virtual void on_shutdown   (const StreamError& err, ShutdownRequest* req);
    virtual void on_eof        ();
    virtual void reset         ();
    virtual bool is_secure     ();

    StreamFilter* next () const { return _next; }
    StreamFilter* prev () const { return _prev; }

    void next (StreamFilter* filter) {
        filter->retain();
        _next = filter;
        filter->_prev = this;
    }

    //friend class Stream;

protected:
    Stream*     handle;
    const char* _type;

    StreamFilter (Stream* h) : handle(h), _type(TYPE), _next(nullptr), _prev(nullptr) {}

    void next_write (WriteRequest* req) {
        // filters may hide initial request and replace it by it's own, so that initial request is never passed to uv_stream_write
        // and therefore won't have 'handle' property set which is required to be set before passed to 'do_on_write'
        _pex_(req)->handle = handle->uvsp();
        _prev ? _prev->write(req) : handle->do_write(req);
    }

    void next_on_connect (const StreamError& err, ConnectRequest* req) {
        _next ? _next->on_connect(err, req) : handle->call_on_connect(err, req, err.code() != ERRNO_ECANCELED);
    }

    void next_on_write (const StreamError& err, WriteRequest* req) {
        _next ? _next->on_write(err, req) : handle->call_on_write(err, req);
    }

    void next_on_read (const string& buf, const StreamError& err) {
        _next ? _next->on_read(buf, err) : handle->call_on_read(buf, err);
    }

    StreamError temp_read_start    () { return handle->_read_start(); }
    void        restore_read_start () { if (!(handle->flags & Stream::SF_WANTREAD)) handle->read_stop(); }

    void connecting (bool val) { handle->connecting(val); }
    void connected  (bool val) { handle->connected(val); }

    ~StreamFilter () {
        if (_next) _next->release();
    }

private:
    StreamFilter* _next;
    StreamFilter* _prev;
};

using StreamFilterSP = panda::iptr<StreamFilter>;

}}
