#pragma once
#include "inc.h"
#include "forward.h"
#include "backend/Backend.h"
#include <vector>
#include <panda/CallbackDispatcher.h>
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

backend::Backend* default_backend     ();
void              set_default_backend (backend::Backend* backend);

struct Loop : Refcnt {
    using Backend     = backend::Backend;
    using BackendLoop = backend::BackendLoop;
    using Handles     = panda::lib::IntrusiveChain<Handle*>;
    using RunMode     = BackendLoop::RunMode;

    static LoopSP global_loop () {
        if (!_global_loop) _init_global_loop();
        return _global_loop;
    }

    static LoopSP default_loop () {
        if (!_default_loop) _init_default_loop();
        return _default_loop;
    }

    Loop (Backend* backend = nullptr) : Loop(backend, BackendLoop::Type::LOCAL) {}

    Backend* backend () const { return _backend; }

    bool is_default () const { return _default_loop == this; }
    bool is_global  () const { return _global_loop == this; }

    uint64_t now         () const { return _impl->now(); }
    void     update_time ()       { _impl->update_time(); }
    bool     alive       () const { return _impl->alive(); }

    virtual bool run         (RunMode = RunMode::DEFAULT);
    virtual void stop        ();
    virtual void handle_fork ();

    bool run_once   () { return run(RunMode::ONCE); }
    bool run_nowait () { return run(RunMode::NOWAIT); }

    const Handles& handles () const { return _handles; }

    uint64_t delay (const BackendLoop::delayed_fn& f, const iptr<Refcnt>& guard = {}) {
        return impl()->delay(f, guard);
    }

    void cancel_delay (uint64_t id) noexcept {
        impl()->cancel_delay(id);
    }

    Resolver* resolver ();

    BackendLoop* impl () { return _impl; }

    void dump () const;

protected:
    ~Loop ();

private:
    Backend*     _backend;
    BackendLoop* _impl;
    Handles      _handles;
    ResolverSP   _resolver;

    Loop (Backend*, BackendLoop::Type);

    void register_handle (Handle* h) {
        _handles.push_back(h);
    }

    void unregister_handle (Handle* h) noexcept {
        _handles.erase(h);
    }

    static LoopSP _global_loop;
    static thread_local LoopSP _default_loop;

    static void _init_global_loop ();
    static void _init_default_loop ();

    friend Handle;
    friend void set_default_backend (backend::Backend*);
};


struct SyncLoop {
    using Backend = backend::Backend;

    static const LoopSP& get (Backend* b) {
        auto& list = loops;
        for (const auto& row : list) if (row.backend == b) return row.loop;
        list.push_back({b, new Loop(b)});
        return list.back().loop;
    }

private:
    struct Item {
        Backend* backend;
        LoopSP   loop;
    };
    static thread_local std::vector<Item> loops;
};

}}
