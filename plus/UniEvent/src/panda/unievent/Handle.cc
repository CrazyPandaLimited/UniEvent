#include "Handle.h"
namespace panda { namespace unievent {

const HandleType Handle::UNKNOWN_TYPE("unknown");

std::ostream& operator<< (std::ostream& out, const HandleType& type) {
    return out << type.name;
}

}}
