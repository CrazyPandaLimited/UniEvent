#pragma once
#include "UVLoop.h"

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVBackend : Backend {
    UVBackend () : Backend("uv") {}

    BackendLoop* new_loop (BackendLoop::Type type) override {
        return new UVLoop(type);
    };
};

}}}}
