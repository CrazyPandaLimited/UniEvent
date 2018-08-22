#include <panda/unievent/global.h>

namespace panda { namespace unievent {

panda::string hostname () {
    panda::string ret(20);
    size_t len = ret.capacity();
    int err = uv_os_gethostname(ret.buf(), &len);
    if (err) {
        if (err != UV_ENOBUFS) throw OperationError(err);
        ret.reserve(len);
        err = uv_os_gethostname(ret.buf(), &len);
        if (err) throw OperationError(err);
    }
    ret.length(len);
    return ret;
}

}}
