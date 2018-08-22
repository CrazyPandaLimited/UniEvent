#pragma once
#include <new>
#include <panda/lib/memory.h>
#include <panda/unievent/Handle.h>
#include <panda/unievent/Request.h>

struct ssl_method_st; typedef ssl_method_st SSL_METHOD;
struct ssl_ctx_st;    typedef ssl_ctx_st SSL_CTX;
struct ssl_st;        typedef ssl_st SSL;

namespace panda { namespace unievent {

class StreamFilter;

class Stream : public virtual Handle {
public:
    using connection_fptr     = void(Stream* handle, const StreamError& err);
    using ssl_connection_fptr = void(Stream* handle, const StreamError& err);
    using read_fptr           = void(Stream* handle, const string& buf, const StreamError& err);
    using eof_fptr            = void(Stream* handle);

    using connect_fptr  = ConnectRequest::connect_fptr;
    using write_fptr    = WriteRequest::write_fptr;
    using shutdown_fptr = ShutdownRequest::shutdown_fptr;

    using connection_fn     = function<connection_fptr>;
    using ssl_connection_fn = function<ssl_connection_fptr>;
    using connect_fn        = function<connect_fptr>;
    using read_fn           = function<read_fptr>;
    using write_fn          = function<write_fptr>;
    using shutdown_fn       = function<shutdown_fptr>;
    using eof_fn            = function<eof_fptr>;

    CallbackDispatcher<connection_fptr>     connection_event;
    CallbackDispatcher<write_fptr>          write_event;
    CallbackDispatcher<shutdown_fptr>       shutdown_event;
    CallbackDispatcher<connect_fptr>        connect_event;
    CallbackDispatcher<eof_fptr>            eof_event;
    CallbackDispatcher<ssl_connection_fptr> ssl_connection_event;

    CallbackDispatcher<read_fptr> read_event;

    bool   readable         () const { return uv_is_readable(uvsp_const()); }
    bool   writable         () const { return uv_is_writable(uvsp_const()); }
    bool   connecting       () const { return flags & SF_CONNECTING; }
    bool   connected        () const { return flags & SF_CONNECTED; }
    bool   shutting_down    () const { return flags & SF_SHUTTING; }
    bool   is_shut_down     () const { return flags & SF_SHUT; }
    bool   want_read        () const { return flags & SF_WANTREAD; }
    size_t write_queue_size () const { return uvsp_const()->write_queue_size; }

    virtual void read_start (read_fn callback = nullptr);
    virtual void read_stop  ();

    virtual void listen     (int backlog = 128, connection_fn callback = nullptr);
    virtual void accept     (Stream* client);
    virtual void shutdown   (ShutdownRequest* req = nullptr);
    virtual void write      (WriteRequest* req);
    virtual void disconnect ();

    void reset () override;

    void shutdown (shutdown_fn callback) { shutdown(new ShutdownRequest(callback)); }

    void write (const string& buf, write_fn callback = nullptr) { write(new WriteRequest(buf, callback)); }

    template <class It>
    void write (It begin, It end, write_fn callback = nullptr) { write(new WriteRequest(begin, end, callback)); }

    void add_filter (StreamFilter* filter);
    StreamFilter* filters () const { return filter_list.head; }

    void use_ssl    (SSL_CTX* context);
    void use_ssl    (const SSL_METHOD* method = nullptr);
    SSL* get_ssl    () const;
    bool is_secure  () const;

    void call_on_connection     (const StreamError& err)                    { on_connection(err); }
    void call_on_ssl_connection (const StreamError& err)                    { on_ssl_connection(err); }
    void call_on_read           (const string& buf, const StreamError& err) { on_read(buf, err); }
    void call_on_eof            ()                                          { on_eof(); }

    void call_on_connect (const StreamError& err, ConnectRequest* req, bool unlock = true);
    void cancel_connect();

    void call_on_write (const StreamError& err, WriteRequest* req) {
        req->event(this, err, req);
        on_write(err, req);
        req->release();
        release();
    }

    void call_on_shutdown (const StreamError& err, ShutdownRequest* req, bool unlock = true) {
        flags &= ~SF_SHUTTING;
        if (!err) flags |= SF_SHUT;
        {
            auto guard = lock_in_callback();
            req->event(this, err, req);
            on_shutdown(err, req);
        }
        req->release();
        if (unlock) async_unlock();
        release();
    }

    friend uv_stream_t* _pex_ (Stream*);
    friend class StreamFilter;

protected:
    Stream () {
        filter_list.head = filter_list.tail = nullptr;
    }

    static const int SF_CONNECTING = HF_LAST << 1;
    static const int SF_CONNECTED  = HF_LAST << 2;
    static const int SF_SHUTTING   = HF_LAST << 3;
    static const int SF_SHUT       = HF_LAST << 4;
    static const int SF_WANTREAD   = HF_LAST << 5;
    static const int SF_READING    = HF_LAST << 6;
    static const int SF_LAST       = SF_READING;

    void connecting (bool val) {
        if (val) {
            flags &= ~SF_CONNECTED;
            flags |= SF_CONNECTING;
        }
        else flags &= ~SF_CONNECTING;
    }

    void connected (bool val) {
        if (val) {
            flags &= ~SF_CONNECTING;
            flags |= SF_CONNECTED;
        }
        else flags &= ~SF_CONNECTED;
    }

    void on_handle_reinit () override;

    virtual void on_connection     (const StreamError& err);
    virtual void on_ssl_connection (const StreamError& err);
    virtual void on_connect        (const StreamError& err, ConnectRequest* req);
    virtual void on_read           (const string& buf, const StreamError& err);
    virtual void on_write          (const StreamError& err, WriteRequest* req);
    virtual void on_shutdown       (const StreamError& err, ShutdownRequest* req);
    virtual void on_eof            ();

    static void uvx_on_connect (uv_connect_t* uvreq, int status);

    ~Stream ();

private:
    struct {
        StreamFilter* head;
        StreamFilter* tail;
    } filter_list;

    uv_stream_t*       uvsp       ()       { return reinterpret_cast<uv_stream_t*>(uvhp); }
    const uv_stream_t* uvsp_const () const { return reinterpret_cast<const uv_stream_t*>(uvhp); }

    StreamError _read_start ();

    void do_write (WriteRequest* req);

    void asyncq_cancel_connect (CommandBase* last_tail);

    static void uvx_on_connection (uv_stream_t* stream, int status);
    static void uvx_on_read       (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void uvx_on_write      (uv_write_t* uvreq, int status);
    static void uvx_on_shutdown   (uv_shutdown_t* uvreq, int status);
};

inline uv_stream_t* _pex_ (Stream* stream) { return stream->uvsp(); }

using StreamSP = iptr<Stream>;

}}
