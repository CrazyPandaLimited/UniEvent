#include "UVLoop.h"

namespace panda { namespace unievent { namespace backend {

struct UVBackend : Backend {
    UVBackend () : Backend("uv") {}

    BackendLoop* new_loop () override {
        return new UVLoop(nullptr);
    };

    BackendLoop* new_global_loop () override {
        auto uvl = uv_default_loop();
        if (!uvl) throw Error("[UVBackend] uv_default_loop() couldn't create a loop");
        return new UVLoop(uvl);
    }

    BackendLoop* new_default_loop () override {
        return new UVLoop(nullptr);
    }
}

}}}
