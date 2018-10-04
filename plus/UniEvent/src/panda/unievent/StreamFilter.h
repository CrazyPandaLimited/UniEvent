#pragma once

#include <panda/unievent/Error.h>
#include <panda/unievent/IntrusiveChain.h>
#include <panda/unievent/Request.h>

namespace panda { namespace unievent {

class Stream;

template <class T> class BasicForwardFilter {
public:
    virtual void connect(ConnectRequest* connect_request) {
        if (self().next_)
            self().next_->connect(connect_request);
    }

    virtual void write(WriteRequest* write_request) {
        if (self().next_)
            self().next_->write(write_request);
    }
    
private:
    T& self() { return static_cast<T&>(*this); }
};

template <class T> class BasicReverseFilter {
public:
    virtual void on_connection(Stream* stream, const CodeError* err) {
        if (self().prev_)
            self().prev_->on_connection(stream, err);
    }
    
    virtual void on_connect(const CodeError* err, ConnectRequest* connect_request) {
        if (self().prev_)
            self().prev_->on_connect(err, connect_request);
    }
    
    virtual void on_write(const CodeError* err, WriteRequest* write_request) {
        if (self().prev_)
            self().prev_->on_write(err, write_request);
    }
    
    virtual void on_read(string& buf, const CodeError* err) {
        if (self().prev_)
            self().prev_->on_read(buf, err);
    }
    
    virtual void on_shutdown(const CodeError* err, ShutdownRequest* shutdown_request) {
        if (self().prev_)
            self().prev_->on_shutdown(err, shutdown_request);
    }
    
    virtual void on_eof() {
        if (self().prev_)
            self().prev_->on_eof();
    }
    
    virtual void on_reinit() {
        if (self().prev_)
            self().prev_->on_reinit();
    }

private:
    T& self() { return static_cast<T&>(*this); }
};

class StreamFilter : public BasicForwardFilter<StreamFilter>,
                     public BasicReverseFilter<StreamFilter>,
                     public virtual Refcnt,
                     public IntrusiveChainNode<iptr<StreamFilter>> {
    friend Stream;

public:
    static const char* TYPE;

    virtual bool is_secure();

protected:
    virtual ~StreamFilter();
    StreamFilter(Stream* h, const char* type);

    CodeError temp_read_start();
    void      restore_read_start();

    using NextFilter = StreamFilter;

    void set_connecting();
    void set_connected(bool success);
    void set_shutdown(bool success);

protected:
    Stream*     handle;
    const char* type_;
};

using StreamFilterSP = panda::iptr<StreamFilter>;

class FrontStreamFilter : public StreamFilter, public AllocatedObject<FrontStreamFilter, true> {
public:
    static const char* TYPE;

    virtual ~FrontStreamFilter();
    FrontStreamFilter(Stream* h);

    StreamFilterSP clone() const override { return StreamFilterSP(new FrontStreamFilter(handle)); };

    void on_connection(Stream* stream, const CodeError* err) override;
    void connect(ConnectRequest* connect_request) override;
    void on_connect(const CodeError* err, ConnectRequest* connect_request) override;
    void write(WriteRequest* write_request) override;
    void on_write(const CodeError* err, WriteRequest* write_request) override;
    void on_read(string& buf, const CodeError* err) override;
    void on_shutdown(const CodeError* err, ShutdownRequest* shutdown_request) override;
    void on_eof() override;
    void on_reinit() override;
};

class BackStreamFilter : public StreamFilter, public AllocatedObject<BackStreamFilter, true> {
public:
    static const char* TYPE;

    virtual ~BackStreamFilter();
    BackStreamFilter(Stream* h);

    StreamFilterSP clone() const override { return StreamFilterSP(new BackStreamFilter(handle)); };

    void on_connection(Stream* stream, const CodeError* err) override;
    void connect(ConnectRequest* connect_request) override;
    void on_connect(const CodeError* err, ConnectRequest* connect_request) override;
    void write(WriteRequest* write_request) override;
    void on_write(const CodeError* err, WriteRequest* write_request) override;
    void on_read(string& buf, const CodeError* err) override;
    void on_shutdown(const CodeError* err, ShutdownRequest* shutdown_request) override;
    void on_eof() override;
    void on_reinit() override;
};

}} // namespace panda::event
