#pragma once
#include "forward.h"
#include "Error.h"
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

struct StreamFilter;
using StreamFilterSP = iptr<StreamFilter>;

struct StreamFilter : Refcnt, panda::lib::IntrusiveChainNode<StreamFilterSP> {
    const void* type     () const { return _type; }
    double      priority () const { return _priority; }

    virtual bool is_secure ();

    virtual void handle_connection (const StreamSP&, const CodeError*);
    virtual void handle_connect    (const CodeError*, const ConnectRequestSP&);
    virtual void handle_read       (string&, const CodeError*);
    virtual void handle_eof        ();
    virtual void handle_shutdown   (const CodeError*, const ShutdownRequestSP&);

    virtual void reset ();

//    virtual void connect       (ConnectRequest*);
//    virtual void write         (WriteRequest*);
//    virtual void on_write      (const CodeError*, WriteRequest*);
//    virtual void on_reinit     ();

protected:
    StreamFilter (Stream* h, const void* type, double priority);

//    CodeError temp_read_start    ();
//    void      restore_read_start ();

    using NextFilter = StreamFilter;

//    void set_connecting();
//    void set_connected(bool success);
//    void set_shutdown(bool success);
//
//    friend Stream;
    Stream*  handle;

    ~StreamFilter () = 0;

private:
    const void*  _type;
    const double _priority;

    template <class T1, class T2, class...Args>
    inline void invoke (const StreamFilterSP& obj, T1 smeth, T2 hmeth, Args&&...args) {
        if (obj) (obj->*smeth)(std::forward<Args>(args)...);
        else     (handle->*hmeth)(std::forward<Args>(args)...);
    }
};

}}
