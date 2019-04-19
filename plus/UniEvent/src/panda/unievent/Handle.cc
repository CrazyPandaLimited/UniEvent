#include "Handle.h"
namespace panda { namespace unievent {

const HandleType Handle::UNKNOWN_TYPE("unknown");

Handle::~Handle () {
    if (!_loop) return; // _init() has never been called (like exception in end class ctor)
    _loop->unregister_handle(this);
    if (_impl) _impl->destroy();
}

std::ostream& operator<< (std::ostream& out, const HandleType& type) {
    return out << type.name;
}

}}
