#include "Loop.h"
#include "Error.h"
#include "Handle.h"
#include "Prepare.h"
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
    _ECTOR();
    if (!backend) backend = default_backend();
    _backend = backend;
    _impl = backend->new_loop(this, type);
}

Loop::~Loop () {
    _EDTOR();
    while (_handles.size()) _handles.front()->destroy();
    delete _impl;
}

void Loop::call_soon (soon_fn f) {
    if (!_soon_handle) {
        _soon_handle = new Prepare(this);
        _soon_handle->start([this](Prepare*) { _call_soon(); });
    }
    else if (!_soon_callbacks.size()) {
        _soon_handle->start();
    }

    _soon_callbacks.push_back(f);
}

void Loop::_call_soon () {
    assert(!_soon_callbacks_reserve.size());
    std::swap(_soon_callbacks, _soon_callbacks_reserve);

    for (auto& f : _soon_callbacks_reserve) if (f) f();

    _soon_callbacks_reserve.clear();
    if (!_soon_callbacks.size()) _soon_handle->stop();
}

void Loop::dump () const {
    for (auto h : _handles) {
        printf("%s(%p)\n", h->type().name, h);
    }
}

//ResolverSP Loop::resolver () {
//    if (!_resolver) _resolver = new Resolver(this);
//    return _resolver;
//}


}}
