#include "inc.h"
#include <panda/unievent/engine/BackendLoop.h>

namespace panda { namespace unievent { namespace backend {

struct UVLoop : BackendLoop {
    UVLoop () : _uvloop(&_uvloop_body) {
        int err = uv_loop_init(_uvloop);
        if (err) throw uvx_code_error(err);
        //_uvloop->data = this;
    }

    ~UVLoop () {
        uv_run(_uvloop, UV_RUN_NOWAIT); // finish all closing handles
        int err = uv_loop_close(_uvloop);
        assert(!err); // unievent doesn't close non-empty loops
    }

    int  run        () override { return uv_run(_uvloop, UV_RUN_DEFAULT); }
    int  run_once   () override { return uv_run(_uvloop, UV_RUN_ONCE); }
    int  run_nowait () override { return uv_run(_uvloop, UV_RUN_NOWAIT); }
    void stop       () override { uv_stop(_uvloop); }

    void handle_fork () override {
        int err = uv_loop_fork(_uvloop);
        if (err) throw uvx_code_error(err);
    }

    uv_loop_t  _uvloop_body;
    uv_loop_t* _uvloop;
};

}}}
