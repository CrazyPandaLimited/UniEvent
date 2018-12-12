#pragma once
#include "Fwd.h"
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

    Loop (Backend* backend = nullptr);

    virtual ~Loop ();

    const Backend* backend () const { return _backend; }

    bool     is_default      () const { return _default_loop == this; }
    bool     is_global       () const { return _global_loop == this; }

    virtual int  run         () { return impl->run(); }
    virtual int  run_once    () { return impl->run_once(); }
    virtual int  run_nowait  () { return impl->run_nowait(); }
    virtual void stop        () { impl->stop(); }
    virtual void handle_fork () { impl->handle_fork(); }

    const Handles& handles () const { return _handles; }

    ResolverSP resolver ();

private:
    Backend*     _backend;
    BackendLoop* impl;
    Handles      _handles;
    ResolverSP   _resolver;

    Loop (Backend*, BackendLoop*);

    static LoopSP _global_loop;
    static thread_local LoopSP _default_loop;

    static void _init_global_loop ();
    static void _init_default_loop ();
};

//int      backend_timeout () const { return uv_backend_timeout(_uvloop); }
//uint64_t now             () const { return uv_now(_uvloop); }
//void     update_time     ()       { uv_update_time(_uvloop); }
//bool     alive           () const { return uv_loop_alive(_uvloop) != 0; }

}}
