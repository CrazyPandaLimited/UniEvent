#include "Loop.h"

#include <thread>

#include "Debug.h"
#include "Resolver.h"

namespace panda { namespace unievent {

Loop* Loop::_global_loop = nullptr;
thread_local Loop* Loop::_default_loop = nullptr;

static std::thread::id main_thread_id = std::this_thread::get_id();

void Loop::_init_global_loop () {
    _global_loop = new Loop(true);
    _global_loop->retain(); // make immortal
}

void Loop::_init_default_loop () {
    if (std::this_thread::get_id() == main_thread_id) _default_loop = global_loop();
    else _default_loop = new Loop();
    _default_loop->retain(); // make immortal
}

Loop::Loop () : closed(false) {
    _ECTOR();
    _uvloop = &_uvloop_body;
    int err = uv_loop_init(_uvloop);
    if (err) throw CodeError(err);
    _uvloop->data = this;
}

// constructor for global default loop
Loop::Loop (bool) : closed(false) {
    _ECTOR();
    _uvloop = uv_default_loop();
    if (!_uvloop) throw Error("Cannot create default loop: uv_default_loop() failed");
    _uvloop->data = this;
}

void Loop::uvx_walk_cb (uv_handle_t* uvh, void* arg) {
    Handle* handle = static_cast<Handle*>(uvh->data);
    walk_fn* callback = static_cast<walk_fn*>(arg);
    (*callback)(handle);
}

int  Loop::run        () { _EDEBUGTHIS("Loop::run)"); return uv_run(_uvloop, UV_RUN_DEFAULT); }
int  Loop::run_once   () { _EDEBUGTHIS("Loop::run)"); return uv_run(_uvloop, UV_RUN_ONCE); }
int  Loop::run_nowait () { _EDEBUGTHIS("Loop::run)"); return uv_run(_uvloop, UV_RUN_NOWAIT); }
void Loop::stop       () { _EDEBUGTHIS("Loop::stop)"); uv_stop(_uvloop); }

void Loop::walk (walk_fn cb) {
    uv_walk(_uvloop, uvx_walk_cb, &cb);
}

CachedResolverSP Loop::default_resolver () {
    if (!resolver) {
        resolver = new CachedResolver(this);
    }
    return resolver;
}

void Loop::close () {
    int err = uv_loop_close(_uvloop);
    if (err) throw CodeError(err);
    closed = true;
    if (is_default()) {
        _default_loop = nullptr;
        release();
    }
    if (is_global()) {
        _global_loop = nullptr;
        release();
    }
}

void Loop::handle_fork () {
    int err = uv_loop_fork(_uvloop);
    if (err) {
        throw CodeError(err);
    }
}

Loop::~Loop () {
    _EDTOR();
    assert(closed);
}

}}
