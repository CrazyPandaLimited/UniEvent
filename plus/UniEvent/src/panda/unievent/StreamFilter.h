#pragma once

#include "Fwd.h"
#include "Error.h"
#include "Request.h"
#include "IntrusiveChain.h"

namespace panda { namespace unievent {

struct StreamFilter : virtual Refcnt, IntrusiveChainNode<iptr<StreamFilter>> {
    const void* type     () const { return _type; }
    double      priority () const { return _priority; }

    virtual bool is_secure ();

    virtual void connect       (ConnectRequest*);
    virtual void write         (WriteRequest*);
    virtual void on_connection (StreamSP, const CodeError*);
    virtual void on_connect    (const CodeError*, ConnectRequest*);
    virtual void on_write      (const CodeError*, WriteRequest*);
    virtual void on_read       (string&, const CodeError*);
    virtual void on_shutdown   (const CodeError*, ShutdownRequest*);
    virtual void on_eof        ();
    virtual void on_reinit     ();

protected:
    StreamFilter (Stream* h, const void* type, double priority);

    CodeError temp_read_start    ();
    void      restore_read_start ();

    using NextFilter = StreamFilter;

    void set_connecting();
    void set_connected(bool success);
    void set_shutdown(bool success);

    friend Stream;
    Stream*  handle;

private:
    const void*  _type;
    const double _priority;
};

struct StreamFilters : IntrusiveChain<StreamFilterSP> {
    StreamFilters (Stream* h) : handle(h) {}

    void connect       (ConnectRequest*);
    void on_connect    (const CodeError*, ConnectRequest*);
    void on_connection (StreamSP, const CodeError*);
    void write         (WriteRequest*);
    void on_write      (const CodeError*, WriteRequest*);
    void on_read       (string&, const CodeError*);
    void on_shutdown   (const CodeError*, ShutdownRequest*);
    void on_eof        ();
    void on_reinit     () {
        if (size()) back()->on_reinit();
    }

private:
    Stream* handle;
};

}}
