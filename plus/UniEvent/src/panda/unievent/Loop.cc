#include "Loop.h"
#include "Error.h"
#include "Handle.h"
//#include "Resolver.h"
#include <panda/unievent/backend/uv.h>
#include <thread>

namespace panda { namespace unievent {

static std::thread::id main_thread_id = std::this_thread::get_id();

static backend::Backend* _default_backend = nullptr;

LoopSP              Loop::_global_loop;
thread_local LoopSP Loop::_default_loop;

backend::Backend* default_backend () {
    return _default_backend ? _default_backend : backend::UV;
}

void set_default_backend (backend::Backend* backend) {
    if (!backend) throw std::invalid_argument("backend can not be nullptr");
    if (Loop::_global_loop || Loop::_default_loop) throw Error("default backend can not be set after global/default loop first used");
    _default_backend = backend;
}

void Loop::_init_global_loop () {
    _global_loop = new Loop(nullptr, BackendLoop::Type::GLOBAL);
}

void Loop::_init_default_loop () {
    if (std::this_thread::get_id() == main_thread_id) _default_loop = global_loop();
    else _default_loop = new Loop(nullptr, BackendLoop::Type::DEFAULT);
}

Loop::Loop (Backend* backend, BackendLoop::Type type) {
    if (!backend) backend = default_backend();
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
