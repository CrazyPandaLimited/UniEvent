#pragma once
#include "inc.h"
#include "forward.h"
#include "backend/Backend.h"
#include "backend/BackendTick.h"
#include <panda/CallbackDispatcher.h>
#include <panda/lib/intrusive_chain.h>
#include <vector>

namespace panda { namespace unievent {

backend::Backend* default_backend     ();
void              set_default_backend (backend::Backend* backend);

struct Loop : Refcnt {
    using Backend      = backend::Backend;
    using BackendLoop  = backend::BackendLoop;
    using BackendTick  = backend::BackendTick;
    using Handles      = panda::lib::IntrusiveChain<Handle*>;
    using delayed_fn   = function<void()>;

    struct Delayer : private backend::ITickListener {
        Delayer (Loop* l) : loop(l), tick(), lastid(0) {}

        uint64_t add    (const delayed_fn& f, const iptr<Refcnt>& guard = {});
        bool     cancel (uint64_t id);

    private:
        struct Callback {
            size_t            id;
            delayed_fn        cb;
            weak_iptr<Refcnt> guard;
        };
        using Callbacks = std::vector<Callback>;

        Loop*        loop;
        BackendTick* tick;
        Callbacks    callbacks;
        Callbacks    reserve;
        uint64_t     lastid;

        void on_tick () override;

        void reset ();

        friend Loop;
    };

    Delayer delayer;

    static LoopSP global_loop () {
        if (!_global_loop) _init_global_loop();
        return _global_loop;
    }

    static LoopSP default_loop () {
        if (!_default_loop) _init_default_loop();
        return _default_loop;
    }

    Loop (Backend* backend = nullptr) : Loop(backend, BackendLoop::Type::LOCAL) {}

    const Backend* backend () const { return _backend; }

    bool is_default () const { return _default_loop == this; }
    bool is_global  () const { return _global_loop == this; }

    uint64_t now         () const { return _impl->now(); }
    void     update_time ()       { _impl->update_time(); }
    bool     alive       () const { return _impl->alive(); }

    virtual bool run         ();
    virtual bool run_once    ();
    virtual bool run_nowait  ();
    virtual void stop        ();
    virtual void handle_fork ();

    const Handles& handles () const { return _handles; }

    uint64_t delay (const delayed_fn& f, const iptr<Refcnt>& guard = {}) { return delayer.add(f, guard); }

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

}}
