#include "Handle.h"
namespace panda { namespace unievent {

const HandleType Handle::UNKNOWN_TYPE("unknown");

Handle::~Handle () {
    _loop->unregister_handle(this);
    _impl->destroy();
}

std::ostream& operator<< (std::ostream& out, const HandleType& type) {
    return out << type.name;
}

}}
