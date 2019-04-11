#include "XSCallback.h"
#include "../compat.h"
using namespace xs::unievent;
using xs::my_perl;

void XSCallback::set (SV* cbsv) {
    if (!cbsv || !SvOK(cbsv)) {
        SvREFCNT_dec(callback);
        callback = nullptr;
        return;
    }

    CV* cv;
    if (!SvROK(cbsv) || SvTYPE(cv = (CV*)SvRV(cbsv)) != SVt_PVCV) croak("[UniEvent] unsupported argument type for setting callback");

    SvREFCNT_dec(callback);
    callback = cv;
    SvREFCNT_inc_simple_void_NN(callback);
}

SV* XSCallback::get () {
    return callback ? newRV((SV*)callback) : SvREFCNT_inc_simple_NN(&PL_sv_undef);
}

bool XSCallback::call (const Object& handle, const Simple& evname, std::initializer_list<Scalar> args) {
    if (!handle) return true; // object is being destroyed

    Sub cv;
    if (evname) cv = handle.method(evname); // default behaviour - call method evname on handle. evname is recommended to be a shared hash string for performance.
    if (!cv) cv = callback;
    if (!cv) return false;

    cv.call(handle.ref(), args);

    return true;
}
