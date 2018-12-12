#include "Loop.h"
#include "Error.h"
#include "Resolver.h"
#include <thread>

namespace panda { namespace unievent {

static std::thread::id main_thread_id = std::this_thread::get_id();

static backend::Backend* _default_backend = nullptr;

Loop* Loop::_global_loop = nullptr;
thread_local Loop* Loop::_default_loop = nullptr;

backend::Backend* default_backend () { return _default_backend; }

static inline backend::Backend* _defback_strict () {
    if (!_default_backend) throw Error("you should set default backend for this operation");
    return _default_backend;
}

void set_default_backend (backend::Backend* backend) {
    if (_defaut_backend) throw Error("you can set default backend only once");
    _defaut_backend = backend;
}

void Loop::_init_global_loop () {
    _global_loop = new Loop(_defback_strict(), _defback_strict()->new_global_loop());
}

void Loop::_init_default_loop () {
    if (std::this_thread::get_id() == main_thread_id) _default_loop = global_loop();
    else _default_loop = new Loop(_defback_strict(), _defback_strict()->new_default_loop());
}

Loop::Loop (Backend* backend) {
    if (!backend) backend = _defback_strict();
    _backend = backend;
    impl = backend->new_loop();
}

// constructor for global/default loop
Loop::Loop (Backend* backend, BackendLoop* bloop) {
    _backend = backend;
    impl = bloop;
}

ResolverSP Loop::resolver () {
    if (!_resolver) _resolver = new Resolver(this);
    return _resolver;
}

Loop::~Loop () {
    _resolver.reset();
    for (auto& handle : _handles) handle->destroy();
    assert(!_handles.size());
    delete backend;
}

}}
