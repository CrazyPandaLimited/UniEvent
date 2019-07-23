#pragma once
#include "inc.h"
#include "forward.h"
#include "backend/Backend.h"
#include <vector>
#include <panda/intrusive_chain.h>
#include <panda/CallbackDispatcher.h>

namespace panda { namespace unievent {

backend::Backend* default_backend     ();
void              set_default_backend (backend::Backend* backend);

struct Loop : Refcnt {
    using Backend  = backend::Backend;
    using LoopImpl = backend::LoopImpl;
    using Handles  = IntrusiveChain<Handle*>;
    using RunMode  = LoopImpl::RunMode;

    static LoopSP global_loop () {
        if (!_global_loop) _init_global_loop();
        return _global_loop;
    }

    static LoopSP default_loop () {
        if (!_default_loop) _init_default_loop();
        return _default_loop;
    }

    Loop (Backend* backend = nullptr) : Loop(backend, LoopImpl::Type::LOCAL) {}

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

    uint64_t delay (const LoopImpl::delayed_fn& f, const iptr<Refcnt>& guard = {}) {
        return impl()->delay(f, guard);
    }

    void cancel_delay (uint64_t id) noexcept {
        impl()->cancel_delay(id);
    }

    Resolver* resolver ();

    LoopImpl* impl () { return _impl; }

    void dump () const;

protected:
    ~Loop ();

private:
    friend Handle; friend Work;
    using Works = IntrusiveChain<WorkSP>;

    Backend*   _backend;
    LoopImpl*  _impl;
    Handles    _handles;
    ResolverSP _resolver;
    Works      _works;

    Loop (Backend*, LoopImpl::Type);

    void register_handle   (Handle* h)          { _handles.push_back(h); }
    void unregister_handle (Handle* h) noexcept { _handles.erase(h); }

    void register_work   (const WorkSP& w)          { _works.push_back(w); }
    void unregister_work (const WorkSP& w) noexcept { _works.erase(w); }

    static LoopSP _global_loop;
    static thread_local LoopSP _default_loop;

    static void _init_global_loop ();
    static void _init_default_loop ();

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

#include "Work.h"
#include "Handle.h"
