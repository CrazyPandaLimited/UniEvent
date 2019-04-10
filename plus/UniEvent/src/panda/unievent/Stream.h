#pragma once
#include "Queue.h"
#include "Timer.h"
#include "Handle.h"
#include "forward.h"
#include "Request.h"
#include "StreamFilter.h"
#include "backend/BackendStream.h"
//#include <new>
//#include <algorithm>
//#include <panda/lib/memory.h>

struct ssl_method_st; typedef ssl_method_st SSL_METHOD;
struct ssl_ctx_st;    typedef ssl_ctx_st SSL_CTX;
struct ssl_st;        typedef ssl_st SSL;

namespace panda { namespace unievent {

struct Stream : virtual Handle, protected backend::IStreamListener {
    using BackendStream   = backend::BackendStream;
    using Filters         = panda::lib::IntrusiveChain<StreamFilterSP>;
    using conn_factory_fn = function<StreamSP()>;
    using connection_fptr = void(const StreamSP& handle, const StreamSP& client, const CodeError* err);
    using connection_fn   = function<connection_fptr>;
    using connect_fptr    = void(const StreamSP& handle, const CodeError* err, const ConnectRequestSP& req);
    using connect_fn      = function<connect_fptr>;
    using read_fptr       = void(const StreamSP& handle, string& buf, const CodeError* err);
    using read_fn         = function<read_fptr>;
    using write_fptr      = void(const StreamSP& handle, const CodeError* err, const WriteRequestSP& req);
    using write_fn        = function<write_fptr>;
    using shutdown_fptr   = void(const StreamSP& handle, const CodeError* err, const ShutdownRequestSP& req);
    using shutdown_fn     = function<shutdown_fptr>;
    using eof_fptr        = void(const StreamSP& handle);
    using eof_fn          = function<eof_fptr>;

    static const int DEFAULT_BACKLOG = 128;

    buf_alloc_fn                        buf_alloc_callback;
    conn_factory_fn                     connection_factory;
    CallbackDispatcher<connection_fptr> connection_event;
    CallbackDispatcher<connect_fptr>    connect_event;
    CallbackDispatcher<read_fptr>       read_event;
    CallbackDispatcher<write_fptr>      write_event;
    CallbackDispatcher<shutdown_fptr>   shutdown_event;
    CallbackDispatcher<eof_fptr>        eof_event;

    string buf_alloc (size_t cap) noexcept override;

    bool   readable         () const { return impl()->readable(); }
    bool   writable         () const { return impl()->writable(); }
    bool   listening        () const { return flags & LISTENING; }
    bool   connecting       () const { return flags & CONNECTING; }
    bool   connected        () const { return flags & CONNECTED; }
    bool   wantread         () const { return flags & WANTREAD; }
    bool   shutting_down    () const { return flags & SHUTTING; }
    bool   is_shut_down     () const { return flags & SHUT; }
//    size_t write_queue_size () const { return uvsp_const()->write_queue_size; }

//    virtual void read_start (read_fn callback = nullptr);
//    virtual void read_stop  ();

    void         listen     (int backlog) { listen(nullptr, backlog); }
    virtual void listen     (connection_fn callback = nullptr, int backlog = DEFAULT_BACKLOG);
    virtual void accept     (const StreamSP& stream);
//    virtual void shutdown   (ShutdownRequest* req = nullptr);
//    virtual void write      (WriteRequest* req);
//    virtual void disconnect ();

    void reset () override;
    void clear () override;

//    void shutdown (shutdown_fn callback) { shutdown(new ShutdownRequest(callback)); }
//
//    void write (const string& buf, write_fn callback = nullptr) { write(new WriteRequest(buf, callback)); }
//
//    template <class It>
//    void write (It begin, It end, write_fn callback = nullptr) { write(new WriteRequest(begin, end, callback)); }
//
//    void attach (WriteRequest* request) {
//        // filters may hide initial request and replace it by it's own, so that initial request is never passed to uv_stream_write
//        // and therefore won't have 'handle' property set which is required to be set before passed to 'do_on_write'
//        _pex_(request)->handle = uvsp();
//    }
//
//    void attach (ConnectRequest* request) {
//        _pex_(request)->handle = uvsp();
//    }
//
//    void use_ssl (SSL_CTX* context);
//    void use_ssl (const SSL_METHOD* method = nullptr);
//
//    SSL* get_ssl   () const;
//    bool is_secure () const;
//
//    void add_filter (const StreamFilterSP&);
//
//    template <typename F>
//    iptr<F>        get_filter ()                 const { return static_pointer_cast<F>(get_filter(F::TYPE)); }
//    StreamFilterSP get_filter (const void* type) const;
//
//    void push_ahead_filter  (const StreamFilterSP& filter) { filters_.insert(filters_.begin(), filter); }
//    void push_behind_filter (const StreamFilterSP& filter) { filters_.insert(filters_.end(), filter); }

    Filters& filters () { return _filters; }

//    void _close () override;
//    void cancel_connect ();
//
//    void do_write (WriteRequest* req);
//
//    virtual void     on_read              (string& buf, const CodeError* err);
//    virtual void     on_write             (const CodeError* err, WriteRequest* req);
//    virtual void     on_shutdown          (const CodeError* err, ShutdownRequest* req);
//    virtual void     on_eof               ();

    using Handle::fileno;
    using Handle::recv_buffer_size;
    using Handle::send_buffer_size;

protected:
    Queue queue;

    Stream () : flags()/*, filters_(this)*/ {
        _ECTOR();
    }

    void accept();

    virtual StreamSP create_connection () = 0;

    virtual void on_connection (const StreamSP& client, const CodeError* err);
    virtual void on_connect    (const CodeError* err, const ConnectRequestSP& req);

//    void set_shutting   ()             { flags &= ~SF_SHUT; flags |= SF_SHUTTING; }
//    void set_shutdown   (bool success) { flags &= ~SF_SHUTTING; flags = success ? flags | SF_SHUT : flags & ~SF_SHUT; }
//    void set_reading    ()             { flags |= SF_READING; }
//    void clear_reading  ()             { flags &= ~SF_READING; }
//    void set_wantread   ()             { flags |= SF_WANTREAD; }
//    void clear_wantread ()             { flags &= ~SF_WANTREAD; }
//
//    void on_handle_reinit () override;

    ~Stream ();

private:
    friend StreamFilter; friend ConnectRequest;

    static const uint32_t CONNECTING = 1;
    static const uint32_t CONNECTED  = 2;
    static const uint32_t SHUTTING   = 4;
    static const uint32_t SHUT       = 8;
    static const uint32_t WANTREAD   = 16;
    static const uint32_t LISTENING  = 32;

    uint8_t flags;
    Filters _filters;

    BackendStream* impl () const { return static_cast<BackendStream*>(_impl); }

    void set_listening  ()             { flags |= LISTENING; }
    void set_connecting ()             { flags &= ~CONNECTED; flags |= CONNECTING; }
    void set_connected  (bool success) { flags &= ~CONNECTING; flags = success ? flags | CONNECTED : flags & ~CONNECTED; }

    template <class T1, class T2, class...Args>
    void invoke (const StreamFilterSP& filter, T1 filter_method, T2 my_method, Args&&...args) {
        if (filter) (filter->*filter_method)(std::forward<Args>(args)...);
        else        (this->*my_method)(std::forward<Args>(args)...);
    }

    void handle_connection          (const CodeError*) override;
    void finalize_handle_connection (const StreamSP& client, const CodeError*);
    void finalize_handle_connect    (const CodeError*, const ConnectRequestSP&);

    void do_reset ();

//    CodeError _read_start ();
//
//    void asyncq_cancel_connect (CommandBase* last_tail);
//
//    void do_on_connect    (const CodeError*, ConnectRequest*);
//    void do_on_read       (string& buf, const CodeError* err) { on_read(buf, err); }
//    void do_on_write      (const CodeError* err, WriteRequest* write_request);
//    void do_on_shutdown   (const CodeError* err, ShutdownRequest* shutdown_request);
//    void do_on_eof        () {
//        set_connected(false);
//        on_eof();
//    }
};

struct ConnectRequest : Request, private backend::IConnectListener {
    CallbackDispatcher<Stream::connect_fptr> event;

protected:
    ConnectRequest (Stream::connect_fn callback, uint64_t timeout = 0) : timeout(timeout), handle(nullptr) {
        _ECTOR();
        if (callback) event.add(callback);
    }

    backend::BackendConnectRequest* impl () const { return static_cast<backend::BackendConnectRequest*>(_impl); }

    //    ~ConnectRequest ();
    //
    //    void set_timer(Timer* timer);
    //
    //    void release_timer();

    void set (Stream* h) {
        handle = h;
        Request::set(h, h->loop()->impl()->new_connect_request(this));
    }

    void exec           () override;
    void handle_connect (const CodeError*) override;

private:
    friend Stream;

    uint64_t timeout;
    TimerSP  timer;
    Stream*  handle;

    void cancel () override;
};

}}
