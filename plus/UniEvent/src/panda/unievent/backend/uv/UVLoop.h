#include "inc.h"
#include <panda/unievent/engine/BackendLoop.h>

namespace panda { namespace unievent { namespace backend {

struct UVLoop : BackendLoop {
    UVLoop (uv_loop_t* uvl) {
        if (uvl) _uvloop = uvl; // for global loop
        else {                  // for others
            _uvloop = &_uvloop_body;
            int err = uv_loop_init(_uvloop);
            if (err) throw uvx_code_error(err);
        }
    }

    ~UVLoop () {
        uv_run(_uvloop, UV_RUN_NOWAIT); // finish all closing handles
        int err = uv_loop_close(_uvloop);
        assert(!err); // unievent should have closed all handles
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

//void Loop::uvx_walk_cb (uv_handle_t* uvh, void* arg) {
//    Handle* handle = static_cast<Handle*>(uvh->data);
//    walk_fn* callback = static_cast<walk_fn*>(arg);
//    (*callback)(handle);
//}

//void Loop::walk (walk_fn cb) {
//    uv_walk(_uvloop, uvx_walk_cb, &cb);
//}

//using walk_fn = function<void(Handle* event)>;
//virtual void walk (walk_fn cb);
//static void uvx_walk_cb (uv_handle_t* handle, void* arg);
//friend uv_loop_t* _pex_ (Loop*);
//inline uv_loop_t* _pex_ (Loop* loop) { return loop->_uvloop; }

}}}
