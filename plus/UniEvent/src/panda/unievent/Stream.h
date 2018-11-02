#pragma once
#include "Handle.h"
#include "Request.h"
#include "StreamFilter.h"

#include <new>
#include <algorithm>
#include <panda/lib/memory.h>

struct ssl_method_st; typedef ssl_method_st SSL_METHOD;
struct ssl_ctx_st;    typedef ssl_ctx_st SSL_CTX;
struct ssl_st;        typedef ssl_st SSL;

namespace panda { namespace unievent {

struct Stream;
using StreamSP = iptr<Stream>;

struct Stream : virtual Handle {
    using connection_factory_fptr = StreamSP();
    using connection_fptr         = void(Stream* handle, StreamSP client, const CodeError* err);
    using read_fptr               = void(Stream* handle, string& buf, const CodeError* err);
    using eof_fptr                = void(Stream* handle);

    using connect_fptr  = ConnectRequest::connect_fptr;
    using write_fptr    = WriteRequest::write_fptr;
    using shutdown_fptr = ShutdownRequest::shutdown_fptr;

    using connection_factory_fn = function<connection_factory_fptr>;
    using connection_fn         = function<connection_fptr>;
    using connect_fn            = function<connect_fptr>;
    using read_fn               = function<read_fptr>;
    using write_fn              = function<write_fptr>;
    using shutdown_fn           = function<shutdown_fptr>;
    using eof_fn                = function<eof_fptr>;

    CallbackDispatcher<connection_fptr>     connection_event;
    CallbackDispatcher<write_fptr>          write_event;
    CallbackDispatcher<shutdown_fptr>       shutdown_event;
    CallbackDispatcher<connect_fptr>        connect_event;
    CallbackDispatcher<eof_fptr>            eof_event;

    CallbackDispatcher<read_fptr> read_event;
    
    connection_factory_fn connection_factory;

    bool   readable         () const { return uv_is_readable(uvsp_const()); }
    bool   writable         () const { return uv_is_writable(uvsp_const()); }
    bool   connecting       () const { return flags & SF_CONNECTING; }
    bool   connected        () const { return flags & SF_CONNECTED; }
    bool   shutting_down    () const { return flags & SF_SHUTTING; }
    bool   is_shut_down     () const { return flags & SF_SHUT; }
    bool   wantread         () const { return flags & SF_WANTREAD; }
    bool   reading          () const { return flags & SF_READING; }
    bool   listening        () const { return flags & SF_LISTENING; }
    size_t write_queue_size () const { return uvsp_const()->write_queue_size; }

    virtual void read_start (read_fn callback = nullptr);
    virtual void read_stop  ();

    virtual void listen     (int backlog = 128, connection_fn callback = nullptr);
    virtual void accept     (const StreamSP& stream);
    virtual void shutdown   (ShutdownRequest* req = nullptr);
    virtual void write      (WriteRequest* req);
    virtual void disconnect ();

    void reset () override;

    void shutdown (shutdown_fn callback) { shutdown(new ShutdownRequest(callback)); }

    void write (const string& buf, write_fn callback = nullptr) { write(new WriteRequest(buf, callback)); }

    template <class It>
    void write (It begin, It end, write_fn callback = nullptr) { write(new WriteRequest(begin, end, callback)); }

    void attach (WriteRequest* request) {
        // filters may hide initial request and replace it by it's own, so that initial request is never passed to uv_stream_write
        // and therefore won't have 'handle' property set which is required to be set before passed to 'do_on_write'
        _pex_(request)->handle = uvsp();
    }
    
    void attach (ConnectRequest* request) {
        _pex_(request)->handle = uvsp();
    }

    void use_ssl (SSL_CTX* context);
    void use_ssl (const SSL_METHOD* method = nullptr);

    SSL* get_ssl   () const;
    bool is_secure () const;
    
    void add_filter (const StreamFilterSP&);

    template <typename F>
    iptr<F>        get_filter ()                 const { return static_pointer_cast<F>(get_filter(F::TYPE)); }
    StreamFilterSP get_filter (const void* type) const;

    void push_ahead_filter  (const StreamFilterSP& filter) { filters_.insert(filters_.begin(), filter); }
    void push_behind_filter (const StreamFilterSP& filter) { filters_.insert(filters_.end(), filter); }

    StreamFilters& filters () { return filters_; }
    
    void _close () override;
    void cancel_connect ();
     
    void do_write (WriteRequest* req);

    virtual void     on_connection        (StreamSP stream, const CodeError* err);
    virtual void     on_connect           (const CodeError* err, ConnectRequest* req);
    virtual void     on_read              (string& buf, const CodeError* err);
    virtual void     on_write             (const CodeError* err, WriteRequest* req);
    virtual void     on_shutdown          (const CodeError* err, ShutdownRequest* req);
    virtual void     on_eof               ();
    virtual StreamSP on_create_connection ();

    friend uv_stream_t* _pex_ (Stream*);

protected:
    Stream(); 
    
    void accept();

    static const uint32_t SF_CONNECTING = HF_LAST << 1;
    static const uint32_t SF_CONNECTED  = HF_LAST << 2;
    static const uint32_t SF_SHUTTING   = HF_LAST << 3;
    static const uint32_t SF_SHUT       = HF_LAST << 4;
    static const uint32_t SF_WANTREAD   = HF_LAST << 5;
    static const uint32_t SF_READING    = HF_LAST << 6;
    static const uint32_t SF_LISTENING  = HF_LAST << 7;
    static const uint32_t SF_LAST       = SF_LISTENING;

    void set_connecting ()             { flags &= ~SF_CONNECTED; flags |= SF_CONNECTING; }
    void set_connected  (bool success) { flags &= ~SF_CONNECTING; flags = success ? flags | SF_CONNECTED : flags & ~SF_CONNECTED; }
    void set_shutting   ()             { flags &= ~SF_SHUT; flags |= SF_SHUTTING; }
    void set_shutdown   (bool success) { flags &= ~SF_SHUTTING; flags = success ? flags | SF_SHUT : flags & ~SF_SHUT; }
    void set_reading    ()             { flags |= SF_READING; }
    void clear_reading  ()             { flags &= ~SF_READING; }
    void set_wantread   ()             { flags |= SF_WANTREAD; }
    void clear_wantread ()             { flags &= ~SF_WANTREAD; }
    void set_listening  ()             { flags |= SF_LISTENING; }

    void on_handle_reinit () override;

    static void uvx_on_connect (uv_connect_t* uvreq, int status);

    ~Stream ();
    
    StreamFilters filters_;

private:
    friend StreamFilter;
    friend StreamFilters;
    friend CommandWrite;
    friend CommandShutdown;
    friend CommandConnect;
    friend CommandConnectPipe;

    uv_stream_t*       uvsp       ()       { return reinterpret_cast<uv_stream_t*>(uvhp); }
    const uv_stream_t* uvsp_const () const { return reinterpret_cast<const uv_stream_t*>(uvhp); }

    CodeError _read_start ();

    void asyncq_cancel_connect (CommandBase* last_tail);

    void do_on_connect    (const CodeError*, ConnectRequest*);
    void do_on_connection (StreamSP stream, const CodeError* err);
    void do_on_read       (string& buf, const CodeError* err) { on_read(buf, err); }
    void do_on_write      (const CodeError* err, WriteRequest* write_request);
    void do_on_shutdown   (const CodeError* err, ShutdownRequest* shutdown_request);
    void do_on_eof        () {
        set_connected(false);
        on_eof();
    }

    static void uvx_on_connection (uv_stream_t* stream, int status);
    static void uvx_on_read       (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void uvx_on_write      (uv_write_t* uvreq, int status);
    static void uvx_on_shutdown   (uv_shutdown_t* uvreq, int status);
};

inline uv_stream_t* _pex_ (Stream* stream) { return stream->uvsp(); }

}}
