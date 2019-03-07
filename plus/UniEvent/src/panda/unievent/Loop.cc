#include "Loop.h"
#include "Error.h"
#include "Handle.h"
#include "Prepare.h"
#include "Resolver.h"
#include <panda/unievent/backend/uv.h>
#include <thread>

#define HOLD(l) LoopSP hold = l; (void)hold;

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

Loop::Loop (Backend* backend, BackendLoop::Type type) : delayer(this) {
    _ECTOR();
    if (!backend) backend = default_backend();
    _backend = backend;
    _impl = backend->new_loop(this, type);
}

Loop::~Loop () {
    _EDTOR();
    _resolver = nullptr;
    delayer.reset();
    assert(!_handles.size());
    delete _impl;
}

bool Loop::run         () { HOLD(this); return _impl->run(); }
bool Loop::run_once    () { HOLD(this); return _impl->run_once(); }
bool Loop::run_nowait  () { HOLD(this); return _impl->run_nowait(); }
void Loop::stop        () { _impl->stop(); }
void Loop::handle_fork () { _impl->handle_fork(); }


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

void Loop::Delayer::reset () {
    if (tick) {
        tick->destroy();
        tick = nullptr;
    }
}

uint64_t Loop::Delayer::add (const delayed_fn& f, const iptr<Refcnt>& guard) {
    if (!tick) tick = loop->impl()->new_tick(this);
    if (!callbacks.size()) tick->start();
    callbacks.push_back({++lastid, f, guard});
    return lastid;
}

template <class T>
static inline bool _delayer_cancel (T& list, uint64_t id) {
    if (!list.size()) return false;
    if (id < list.front().id) return false;
    size_t idx = id - list.front().id;
    if (idx >= list.size()) return false;
    list[idx].cb = nullptr;
    return true;
}

bool Loop::Delayer::cancel (uint64_t id) {
    return _delayer_cancel(callbacks, id) || _delayer_cancel(reserve, id);
}

void Loop::Delayer::on_tick () {
    HOLD(loop);
    assert(!reserve.size());
    std::swap(callbacks, reserve);

    for (auto& row : reserve) {
        if (row.guard.weak_count() && !row.guard) continue; // skip callbacks with guard destroyed
        if (row.cb) row.cb();
    }

    reserve.clear();
    if (!callbacks.size()) tick->stop();
}

}}
