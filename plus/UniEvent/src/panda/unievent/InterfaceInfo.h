#pragma once
#include <panda/unievent/Error.h>

namespace panda { namespace unievent {

typedef uv_interface_address_t interface_address_t;

class InterfaceInfo {
public:
    InterfaceInfo () : addresses(nullptr), cnt(0) {
        refresh();
    }

    virtual void refresh ();

    const interface_address_t* list  () const { return addresses; }
    int                        count () const { return cnt; }

    virtual ~InterfaceInfo ();

private:
    interface_address_t* addresses;
    int cnt;
};

}}
