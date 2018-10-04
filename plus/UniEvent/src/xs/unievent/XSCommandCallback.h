#pragma once
#include <xs.h>
#include <xs/unievent/XSCallback.h>
#include <panda/unievent/Command.h>

namespace xs { namespace unievent {

class XSCommandCallback : public panda::unievent::CommandCallback {
public:
    XSCallback xscb;
    SV* handle_rv;

    XSCommandCallback (pTHX_ SV* handle_rv) : CommandCallback(nullptr), handle_rv(handle_rv) {
        SvREFCNT_inc_simple_void_NN(handle_rv);
    }

    virtual void run    ();
    virtual void cancel ();

    ~XSCommandCallback ();
};

}}
