#pragma once
#include "Queue.h"
#include "Timer.h"
#include "Handle.h"
#include "forward.h"
#include "Request.h"
#include "StreamFilter.h"
#include "backend/BackendStream.h"

struct ssl_method_st; typedef ssl_method_st SSL_METHOD;
struct ssl_ctx_st;    typedef ssl_ctx_st SSL_CTX;
struct ssl_st;        typedef ssl_st SSL;

namespace panda { namespace unievent {

struct Stream : virtual Handle, protected backend::IStreamListener {
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
    bool   established      () const { return flags & ESTABLISHED; }
    bool   connected        () const { return flags & CONNECTED; }
    bool   wantread         () const { return !(flags & DONTREAD); }
    bool   shutting_down    () const { return flags & SHUTTING; }
    bool   is_shut_down     () const { return flags & SHUT; }
//    size_t write_queue_size () const { return uvsp_const()->write_queue_size; }

    void         listen   (int backlog) { listen(nullptr, backlog); }
    virtual void listen   (connection_fn callback = nullptr, int backlog = DEFAULT_BACKLOG);
    virtual void write    (const WriteRequestSP&);
    /*INL*/ void write    (const string& buf, write_fn callback = nullptr);
    template <class It>
    /*INL*/ void write    (It begin, It end, write_fn callback = nullptr);
    virtual void shutdown (const ShutdownRequestSP&);
    /*INL*/ void shutdown (shutdown_fn callback = {});

    void read_start () {
        set_wantread(true);
        auto err = _read_start();
        if (err) throw err;
    }

    void read_stop ();

    virtual void disconnect ();

    void reset () override;
    void clear () override;

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

    optional<fd_t> fileno () const { return _impl ? impl()->fileno() : optional<fd_t>(); }

    int  recv_buffer_size () const    { return impl()->recv_buffer_size(); }
    void recv_buffer_size (int value) { impl()->recv_buffer_size(value); }
    int  send_buffer_size () const    { return impl()->send_buffer_size(); }
    void send_buffer_size (int value) { impl()->send_buffer_size(value); }

protected:
    Queue queue;

    Stream () : flags() {
        _ECTOR();
    }

    virtual void accept ();
    virtual void accept (const StreamSP& client);

    virtual StreamSP create_connection () = 0;

    virtual void on_connection (const StreamSP& client, const CodeError* err);
    virtual void on_connect    (const CodeError* err, const ConnectRequestSP& req);
    virtual void on_read       (string& buf, const CodeError* err);
    virtual void on_write      (const CodeError* err, const WriteRequestSP& req);
    virtual void on_eof        ();
    virtual void on_shutdown   (const CodeError* err, const ShutdownRequestSP& req);

    void set_listening   () { flags |= LISTENING; }
    void set_connecting  () { flags |= CONNECTING; }
    void set_established () { flags |= ESTABLISHED; }

    CodeError set_connect_result (const CodeError* err) {
        set_connected(!err);
        if (!err && wantread()) return _read_start();
        else return *err;
    }

    void set_connected (bool ok) {
        flags &= ~CONNECTING;
        if (ok) flags |= CONNECTED|ESTABLISHED;
        else    flags &= ~CONNECTED;
    }

    void set_wantread (bool on) { on ? (flags &= ~DONTREAD) : (flags |= DONTREAD); }
    void set_reading  (bool on) { on ? (flags |= READING) : (flags &= ~READING); }
    void set_shutting ()        { flags |= SHUTTING; }
    void set_shutdown (bool ok) { flags &= ~SHUTTING; ok ? (flags |= SHUT) : (flags &= ~SHUT); }

    ~Stream ();

private:
    friend StreamFilter; friend ConnectRequest; friend WriteRequest; friend ShutdownRequest; friend struct DisconnectRequest;

    static const uint32_t LISTENING   = 1;
    static const uint32_t CONNECTING  = 2;
    static const uint32_t ESTABLISHED = 4; // physically connected
    static const uint32_t CONNECTED   = 8; // logically connected
    static const uint32_t DONTREAD    = 16;
    static const uint32_t READING     = 32;
    static const uint32_t SHUTTING    = 64;
    static const uint32_t SHUT        = 128;

    uint8_t flags;
    Filters _filters;

    backend::BackendStream* impl () const { return static_cast<backend::BackendStream*>(Handle::impl()); }

    bool reading () const { return flags & READING; }

    template <class T1, class T2, class...Args>
    void invoke (const StreamFilterSP& filter, T1 filter_method, T2 my_method, Args&&...args) {
        if (filter) (filter->*filter_method)(std::forward<Args>(args)...);
        else        (this->*my_method)(std::forward<Args>(args)...);
    }

    void handle_connection          (const CodeError*) override;
    void finalize_handle_connection (const StreamSP& client, const CodeError*);
    void finalize_handle_connect    (const CodeError*, const ConnectRequestSP&);
    void handle_read                (string&, const CodeError*) override;
    void finalize_handle_read       (string& buf, const CodeError* err) { on_read(buf, err); }
    void finalize_write             (const WriteRequestSP&);
    void finalize_handle_write      (const CodeError*, const WriteRequestSP&);
    void handle_eof                 () override;
    void finalize_handle_eof        () { set_connected(false); on_eof(); }
    void finalize_handle_shutdown   (const CodeError*, const ShutdownRequestSP&);

    void _reset ();
    void _clear ();

    CodeError _read_start ();
};


struct ConnectRequest : Request, private backend::IConnectListener {
    CallbackDispatcher<Stream::connect_fptr> event;

protected:
    uint64_t timeout;
    TimerSP  timer;
    Stream*  handle;

    friend Stream;

    ConnectRequest (Stream::connect_fn callback = {}, uint64_t timeout = 0) : timeout(timeout), handle(nullptr) {
        _ECTOR();
        if (callback) event.add(callback);
    }

    backend::BackendConnectRequest* impl () {
        if (!_impl) _impl = handle->impl()->new_connect_request(this);
        return static_cast<backend::BackendConnectRequest*>(_impl);
    }

    void set (Stream* h) {
        handle = h;
        Request::set(h);
    }

    void exec           () override = 0;
    void cancel         () override;
    void handle_connect (const CodeError*) override;
};


struct WriteRequest : BufferRequest, lib::AllocatedObject<WriteRequest>, private backend::IWriteListener {
    CallbackDispatcher<Stream::write_fptr> event;

    using BufferRequest::BufferRequest;

private:
    friend Stream;
    Stream* handle;

    void set (Stream* h) {
        handle = h;
        Request::set(h);
    }

    backend::BackendWriteRequest* impl () {
        if (!_impl) _impl = handle->impl()->new_write_request(this);
        return static_cast<backend::BackendWriteRequest*>(_impl);
    }

    void exec         () override;
    void cancel       () override;
    void handle_write (const CodeError*) override;
};


struct ShutdownRequest : Request, private backend::IShutdownListener {
    CallbackDispatcher<Stream::shutdown_fptr> event;

    ShutdownRequest (Stream::shutdown_fn callback = {}) {
        _ECTOR();
        if (callback) event.add(callback);
    }

private:
    friend Stream;
    Stream* handle;

    void set (Stream* h) {
        handle = h;
        Request::set(h);
    }

    backend::BackendShutdownRequest* impl () {
        if (!_impl) _impl = handle->impl()->new_shutdown_request(this);
        return static_cast<backend::BackendShutdownRequest*>(_impl);
    }

    void exec            () override;
    void cancel          () override;
    void handle_shutdown (const CodeError*) override;
};


struct DisconnectRequest : Request {
    DisconnectRequest (Stream* h) : handle(h) {
        _ECTOR();
        set(h);
    }

private:
    Stream* handle;

    void exec   () override;
    void cancel () override;
};


inline void Stream::write (const string& data, write_fn callback) {
    auto req = new WriteRequest(data);
    if (callback) req->event.add(callback);
    write(req);
}

template <class It>
inline void Stream::write (It begin, It end, write_fn callback) {
    auto req = new WriteRequest(begin, end);
    if (callback) req->event.add(callback);
    write(req);
}

inline void Stream::shutdown (shutdown_fn callback) { shutdown(new ShutdownRequest(callback)); }

}}
