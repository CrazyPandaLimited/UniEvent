#pragma once
#include "Fwd.h"
#include "Error.h"
#include "backend/Backend.h"

namespace panda { namespace unievent {

struct Loop : Refcnt {
    using backend::Backend;
    using backend::BackendLoop;

    static Backend* default_backend () { return _default_backend; }

    static void set_default_backend (Backend* backend);

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

    //int      backend_timeout () const { return uv_backend_timeout(_uvloop); }
    //uint64_t now             () const { return uv_now(_uvloop); }
    //void     update_time     ()       { uv_update_time(_uvloop); }
    bool     is_default      () const { return _default_loop == this; }
    bool     is_global       () const { return _global_loop == this; }
    //bool     alive           () const { return uv_loop_alive(_uvloop) != 0; }

    virtual int  run         () { backend->run(); }
    virtual int  run_once    () { backend->run_once(); }
    virtual int  run_nowait  () { backend->run_nowait(); }
    virtual void stop        () { backend->stop(); }
    virtual void handle_fork () { backend->handle_fork(); }

    ResolverSP resolver();

private:
    BackendLoop* backend;
    ResolverSP resolver_;

    Loop (BackendLoop*);

    static Backend* _defaut_backend;
    static LoopSP _global_loop;
    static thread_local LoopSP _default_loop;

    static void _init_global_loop ();
    static void _init_default_loop ();

};

//using walk_fn = function<void(Handle* event)>;
//virtual void walk (walk_fn cb);
//static void uvx_walk_cb (uv_handle_t* handle, void* arg);
//friend uv_loop_t* _pex_ (Loop*);
//inline uv_loop_t* _pex_ (Loop* loop) { return loop->_uvloop; }

}}
