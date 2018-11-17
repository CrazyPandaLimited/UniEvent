#include "Loop.h"
#include "Resolver.h"
#include <thread>

namespace panda { namespace unievent {

Loop* Loop::_global_loop = nullptr;
thread_local Loop* Loop::_default_loop = nullptr;

static std::thread::id main_thread_id = std::this_thread::get_id();

void Loop::set_default_backend (Backend* backend) {
    if (_defaut_backend) throw Error("you can set default backend only once");
    _defaut_backend = backend;
}

void Loop::_init_global_loop () {
    _global_loop = new Loop(true);
}

void Loop::_init_default_loop () {
    if (std::this_thread::get_id() == main_thread_id) _default_loop = global_loop();
    else _default_loop = new Loop();
}

Loop::Loop () : closed(false) {
//    _uvloop = &_uvloop_body;
//    int err = uv_loop_init(_uvloop);
//    if (err) throw CodeError(err);
//    _uvloop->data = this;
}

// constructor for global default loop
Loop::Loop (bool) : closed(false) {
    _uvloop = uv_default_loop();
    if (!_uvloop) throw Error("Cannot create default loop: uv_default_loop() failed");
    _uvloop->data = this;
}

void Loop::uvx_walk_cb (uv_handle_t* uvh, void* arg) {
    Handle* handle = static_cast<Handle*>(uvh->data);
    walk_fn* callback = static_cast<walk_fn*>(arg);
    (*callback)(handle);
}

int  Loop::run        () { _EDEBUGTHIS(); return uv_run(_uvloop, UV_RUN_DEFAULT); }
int  Loop::run_once   () { _EDEBUGTHIS(); return uv_run(_uvloop, UV_RUN_ONCE); }
int  Loop::run_nowait () { _EDEBUGTHIS(); return uv_run(_uvloop, UV_RUN_NOWAIT); }

void Loop::stop() {
    _EDEBUGTHIS();
    if (resolver_) {
        resolver_->stop();
    }

    uv_stop(_uvloop);
}

//void Loop::walk (walk_fn cb) {
//    uv_walk(_uvloop, uvx_walk_cb, &cb);
//}

ResolverSP Loop::resolver () {
    if (!resolver_) {
        resolver_ = new Resolver(this);
    }
    return resolver_;
}

void Loop::close () {
    _EDEBUG();
    
    // give resolver a chance to stop
    run_nowait();

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

Loop::~Loop () {
    delete backend;
}

}}
