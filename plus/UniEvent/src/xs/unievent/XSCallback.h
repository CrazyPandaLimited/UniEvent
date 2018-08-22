#pragma once
#include <xs.h>

namespace xs { namespace unievent {

class XSCallback {
    mTHX;
public:
    CV* callback;

    XSCallback (pTHX) : mTHXa(aTHX) callback(nullptr) {}

    void set  (SV* callback_rv);
    SV*  get  ();
    bool call (const Object& handle, const Simple& evname, std::initializer_list<Scalar> args = {});

    ~XSCallback () {
        SvREFCNT_dec(callback); // release callback
    }
};

}}
