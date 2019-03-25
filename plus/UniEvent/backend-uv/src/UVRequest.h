#pragma once
#include "inc.h"

namespace panda { namespace unievent { namespace backend { namespace uv {

template <class Base>
struct UVRequest : Base {
    bool active;

protected:
    using Base::Base;

    uv_req_t* uvrp;

    void _init (void* p) {
        uvrp = static_cast<uv_req_t*>(p);
        uvrp->data = static_cast<BackendRequest*>(this);
        active = false;
    }
};

}}}}
