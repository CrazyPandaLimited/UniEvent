#pragma once
#include "UVLoop.h"

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVBackend : Backend {
    UVBackend () : Backend("uv") {}

    BackendLoop* new_loop (Loop* frontend, BackendLoop::Type type) override {
        return new UVLoop(frontend, type);
    };
};

}}}}
