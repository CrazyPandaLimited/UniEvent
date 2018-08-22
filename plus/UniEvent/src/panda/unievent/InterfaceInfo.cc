#include <panda/unievent/InterfaceInfo.h>
using namespace panda::unievent;

#define _II_FREE_ if (cnt) { uv_free_interface_addresses(addresses, cnt); cnt = 0; addresses = nullptr; }

void InterfaceInfo::refresh () {
    _II_FREE_;
    int err = uv_interface_addresses(&addresses, &cnt);
    if (err) throw OperationError(err);
}

InterfaceInfo::~InterfaceInfo () {
    _II_FREE_;
}
