#include "Loop.h"
#include "Error.h"
#include "Handle.h"
#include "Prepare.h"
#include "Resolver.h"
#include <panda/unievent/backend/uv.h>
#include <thread>

namespace panda { namespace unievent {

static std::thread::id main_thread_id = std::this_thread::get_id();

static backend::Backend* _default_backend = nullptr;

LoopSP              Loop::_global_loop;
thread_local LoopSP Loop::_default_loop;

thread_local std::vector<SyncLoop::Item> SyncLoop::loops;

backend::Backend* default_backend () {
    return _default_backend ? _default_backend : backend::UV;
}

void set_default_backend (backend::Backend* backend) {
    if (!backend) throw std::invalid_argument("backend can not be nullptr");
    if (Loop::_global_loop || Loop::_default_loop) throw Error("default backend can not be set after global/default loop first used");
    _default_backend = backend;
}

void Loop::_init_global_loop () {
    _global_loop = new Loop(nullptr, LoopImpl::Type::GLOBAL);
}

void Loop::_init_default_loop () {
    if (std::this_thread::get_id() == main_thread_id) _default_loop = global_loop();
    else _default_loop = new Loop(nullptr, LoopImpl::Type::DEFAULT);
}

Loop::Loop (Backend* backend, LoopImpl::Type type) {
    _ECTOR();
    if (!backend) backend = default_backend();
    _backend = backend;
    _impl = backend->new_loop(type);
}

Loop::~Loop () {
    _EDTOR();
    _resolver = nullptr;
    assert(!_handles.size());
    delete _impl;
}

bool Loop::run (RunMode mode) {
    LoopSP hold = this; (void)hold;
    return _impl->run(mode);
}

void Loop::stop () {
    _impl->stop();
}

void Loop::handle_fork () {
    _impl->handle_fork();
}

void Loop::dump () const {
    for (auto h : _handles) {
        printf("%p %s%s [%s%s]\n",
            h,
            h->active() && !h->weak() ? "": "-",
            h->type().name,
            h->active() ? "A" : "",
            h->weak()   ? "W" : ""
        );
    }
}

Resolver* Loop::resolver () {
    if (!_resolver) _resolver = Resolver::create_loop_resolver(this); // does not hold strong backref to loop
    return _resolver.get();
}

}}
