#pragma once
#include <vector>
#include "forward.h"
#include "backend/Backend.h"
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

backend::Backend* default_backend     ();
void              set_default_backend (backend::Backend* backend);

struct Loop : Refcnt {
    using Backend     = backend::Backend;
    using BackendLoop = backend::BackendLoop;
    using Handles     = panda::lib::IntrusiveChain<Handle*>;
    using soon_fn     = function<void()>;

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

    virtual bool run         () { return _impl->run(); }
    virtual bool run_once    () { return _impl->run_once(); }
    virtual bool run_nowait  () { return _impl->run_nowait(); }
    virtual void stop        () { _impl->stop(); }
    virtual void handle_fork () { _impl->handle_fork(); }

    const Handles& handles () const { return _handles; }

    void call_soon (soon_fn f);

    //ResolverSP resolver ();

    BackendLoop* impl () const { return _impl; }

private:
    using SoonCallbacks = std::vector<soon_fn>;

    Backend*      _backend;
    BackendLoop*  _impl;
    Handles       _handles;
    PrepareSP     _soon_handle;
    SoonCallbacks _soon_callbacks;
    SoonCallbacks _soon_callbacks_reserve;
    //ResolverSP    _resolver;

    Loop (Backend*, BackendLoop::Type);

    void register_handle (Handle* h) {
        _handles.push_back(h);
    }

    void unregister_handle (Handle* h) {
        _handles.erase(h);
    }

    void _call_soon ();

    static LoopSP _global_loop;
    static thread_local LoopSP _default_loop;

    static void _init_global_loop ();
    static void _init_default_loop ();

    friend void set_default_backend (backend::Backend*);
    friend class Handle;
};

}}
