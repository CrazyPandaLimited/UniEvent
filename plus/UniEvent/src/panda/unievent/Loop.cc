#include "Loop.h"
#include "Error.h"
#include "Handle.h"
//#include "Resolver.h"
#include <thread>

namespace panda { namespace unievent {

static std::thread::id main_thread_id = std::this_thread::get_id();

static backend::Backend* _default_backend = nullptr;

LoopSP              Loop::_global_loop;
thread_local LoopSP Loop::_default_loop;

backend::Backend* default_backend () { return _default_backend; }

static inline backend::Backend* _defback_strict () {
    if (!_default_backend) throw Error("you should set default backend for this operation");
    return _default_backend;
}

void set_default_backend (backend::Backend* backend) {
    if (_default_backend) throw Error("you can set default backend only once");
    _default_backend = backend;
}

void Loop::_init_global_loop () {
    _global_loop = new Loop(_default_backend, BackendLoop::Type::GLOBAL);
}

void Loop::_init_default_loop () {
    if (std::this_thread::get_id() == main_thread_id) _default_loop = global_loop();
    else _default_loop = new Loop(_default_backend, BackendLoop::Type::DEFAULT);
}

Loop::Loop (Backend* backend, BackendLoop::Type type) {
    if (!backend) backend = _defback_strict();
    _backend = backend;
    _impl = backend->new_loop(this, type);
}

//ResolverSP Loop::resolver () {
//    if (!_resolver) _resolver = new Resolver(this);
//    return _resolver;
//}

Loop::~Loop () {
    while (_handles.size()) _handles.front()->destroy();
    delete _impl;
}

}}
