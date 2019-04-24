#pragma once
#include "forward.h"
#include "Error.h"
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

struct StreamFilter : Refcnt, panda::lib::IntrusiveChainNode<StreamFilterSP> {
    const void* type     () const { return _type; }
    double      priority () const { return _priority; }

    virtual bool is_secure ();

    virtual void tcp_connect       (const TcpConnectRequestSP&);
    virtual void handle_connection (const StreamSP&, const CodeError&);
    virtual void handle_connect    (const CodeError&, const ConnectRequestSP&);
    virtual void handle_read       (string&, const CodeError&);
    virtual void write             (const WriteRequestSP&);
    virtual void handle_write      (const CodeError&, const WriteRequestSP&);
    virtual void handle_eof        ();
    virtual void handle_shutdown   (const CodeError&, const ShutdownRequestSP&);

    virtual void reset ();

protected:
    using NextFilter = StreamFilter;

    StreamFilter (Stream* h, const void* type, double priority);

    CodeError priority_read_start ();
    void      priority_read_stop  ();

    void subreq_tcp_connect (const RequestSP& parent, const TcpConnectRequestSP& subreq);
    void subreq_write       (const RequestSP& parent, const WriteRequestSP& subreq);

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
