#include "UVLoop.h"

namespace panda { namespace unievent { namespace backend {

struct UVBackend : Backend {
    UVBackend () : Backend("uv") {}

    BackendLoop* new_loop () override {
        return new UVLoop();
    };
}

}}}
