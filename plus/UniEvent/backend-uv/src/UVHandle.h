#pragma once
#include "inc.h"
#include <panda/unievent/backend/BackendHandle.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

template <class Base>
struct UVHandle : Base {
protected:
    using Base::Base;

    void _init (void* p) {
        uvhp = static_cast<uv_handle_t*>(p);
        uvhp->data = static_cast<BackendHandle*>(this);
    }

    BackendLoop* loop () const override {
        return reinterpret_cast<BackendLoop*>(uvhp->loop->data);
    }

    void destroy () override {
        throw "pohsle na huy";
    }

    uv_handle_t* uvhp;
};

}}}}
