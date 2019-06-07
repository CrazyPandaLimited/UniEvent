#pragma once
#include "forward.h"
#include "Error.h"
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

struct StreamFilter : Refcnt, panda::lib::IntrusiveChainNode<StreamFilterSP> {
    const void* type     () const { return _type; }
    double      priority () const { return _priority; }

    virtual void handle_connection (const StreamSP&, const CodeError&, const AcceptRequestSP&);
    virtual void tcp_connect       (const TcpConnectRequestSP&);
    virtual void pipe_connect      (const PipeConnectRequestSP&);
    virtual void handle_connect    (const CodeError&, const ConnectRequestSP&);
    virtual void handle_read       (string&, const CodeError&);
    virtual void write             (const WriteRequestSP&);
    virtual void handle_write      (const CodeError&, const WriteRequestSP&);
    virtual void handle_eof        ();
    virtual void shutdown          (const ShutdownRequestSP&);
    virtual void handle_shutdown   (const CodeError&, const ShutdownRequestSP&);

    virtual void listen () { if (next) next->listen(); }
    virtual void reset  () { if (next) next->reset(); }

protected:
    using NextFilter = StreamFilter;

    StreamFilter (Stream* h, const void* type, double priority);

    CodeError read_start ();
    void      read_stop  ();

    void subreq_tcp_connect  (const StreamRequestSP& parent, const TcpConnectRequestSP&);
    void subreq_pipe_connect (const StreamRequestSP& parent, const PipeConnectRequestSP&);
    void subreq_write        (const StreamRequestSP& parent, const WriteRequestSP&);
    void subreq_done         (const StreamRequestSP&);

    Stream*  handle;

private:
    const void*  _type;
    const double _priority;
};

}}
