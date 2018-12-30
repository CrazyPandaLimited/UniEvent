#pragma once
#include "forward.h"
#include "IntrusiveChain.h"
#include "backend/Backend.h"

namespace panda { namespace unievent {

backend::Backend* default_backend     ();
void              set_default_backend (backend::Backend* backend);

struct Loop : Refcnt {
    using Backend     = backend::Backend;
    using BackendLoop = backend::BackendLoop;
    using Handles     = IntrusiveChain<Handle*>;

    static LoopSP global_loop () {
        if (!_global_loop) _init_global_loop();
        return _global_loop;
    }

    static LoopSP default_loop () {
        if (!_default_loop) _init_default_loop();
        return _default_loop;
    }

    Loop (Backend* backend = nullptr) : Loop(backend, BackendLoop::Type::LOCAL) {}

    virtual ~Loop ();

    const Backend* backend () const { return _backend; }

    bool is_default () const { return _default_loop == this; }
    bool is_global  () const { return _global_loop == this; }

    uint64_t now         () const { return _impl->now(); }
    void     update_time ()       { _impl->update_time(); }
    bool     alive       () const { return _impl->alive(); }

    virtual int  run         () { return _impl->run(); }
    virtual int  run_once    () { return _impl->run_once(); }
    virtual int  run_nowait  () { return _impl->run_nowait(); }
    virtual void stop        () { _impl->stop(); }
    virtual void handle_fork () { _impl->handle_fork(); }

    const Handles& handles () const { return _handles; }

    //ResolverSP resolver ();

    BackendLoop* impl () const { return _impl; }

private:
    Backend*     _backend;
    BackendLoop* _impl;
    Handles      _handles;
    //ResolverSP   _resolver;

    Loop (Backend*, BackendLoop::Type);

    void register_handle (Handle* h) {
        _handles.push_front(h);
    }

    static LoopSP _global_loop;
    static thread_local LoopSP _default_loop;

    static void _init_global_loop ();
    static void _init_default_loop ();

    friend void set_default_backend (backend::Backend*);
    friend class Handle;
};

}}
