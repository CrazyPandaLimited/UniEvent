#pragma once
#include "Loop.h"
#include "backend/BackendWork.h"
#include <panda/lib/memory.h>

namespace panda { namespace unievent {

struct Work : Refcnt, lib::IntrusiveChainNode<WorkSP>, lib::AllocatedObject<Work>, private backend::IWorkListener {
    using BackendWork   = backend::BackendWork;
    using work_fn       = function<void(Work*)>;
    using after_work_fn = function<void(const WorkSP&, const CodeError&)>;

    work_fn       work_cb;
    after_work_fn after_work_cb;

    static WorkSP queue (const work_fn&, const after_work_fn&, const LoopSP& = Loop::default_loop());

    Work (const LoopSP& loop = Loop::default_loop()) : _loop(loop), _impl(), _active() {}

    const LoopSP& loop () const { return _loop; }

    virtual void queue  ();
    virtual bool cancel ();

    ~Work () {
        if (_impl) assert(_impl->destroy());
    }

protected:
    virtual void on_work       ();
    virtual void on_after_work (const CodeError& err);

private:
    LoopSP       _loop;
    BackendWork* _impl;
    bool         _active;

    void handle_work       () override;
    void handle_after_work (const CodeError& err) override;

    BackendWork* impl () {
        if (!_impl) _impl = _loop->impl()->new_work(this);
        return _impl;
    }
};

}}
