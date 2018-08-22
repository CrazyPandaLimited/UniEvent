#pragma once
#include <functional>
#include <panda/CallbackDispatcher.h>
#include <panda/refcnt.h>
#include <panda/unievent/Error.h>

namespace panda { namespace unievent {

class Handle;

class Loop : public virtual panda::Refcnt {
public:
    using walk_fn = function<void(Handle* event)>;

    Loop ();

    int      backend_timeout () const { return uv_backend_timeout(_uvloop); }
    uint64_t now             () const { return uv_now(_uvloop); }
    void     update_time     ()       { uv_update_time(_uvloop); }
    bool     is_default      () const { return default_loop() == this; }
    bool     is_global       () const { return global_loop() == this; }
    bool     alive           () const { return uv_loop_alive(_uvloop) != 0; }

    virtual int  run         ();
    virtual int  run_once    ();
    virtual int  run_nowait  ();
    virtual void stop        ();
    virtual void close       ();
    virtual void handle_fork ();

    virtual void walk (walk_fn cb);

    void release () {
        if (refcnt() <= 1 && !closed) close();
        Refcnt::release();
    }

    static Loop* global_loop () {
        if (!_global_loop) _init_global_loop();
        return _global_loop;
    }

    static Loop* default_loop () {
        if (!_default_loop) _init_default_loop();
        return _default_loop;
    }

    friend uv_loop_t* _pex_ (Loop*);

protected:
    virtual ~Loop (); // protected dtor prevents from creating Loop objects on stacks / as class members / etc

private:
    uv_loop_t  _uvloop_body;
    uv_loop_t* _uvloop;
    bool closed;

    Loop (bool);

    static Loop* _global_loop;
    static thread_local Loop* _default_loop;

    static void _init_global_loop ();
    static void _init_default_loop ();

    static void uvx_walk_cb (uv_handle_t* handle, void* arg);
};

inline uv_loop_t* _pex_ (Loop* loop) { return loop->_uvloop; }

using LoopSP = iptr<Loop>;

void refcnt_dec (const Loop* o);
inline void refcnt_dec (Loop* o) { o->release(); }

}}
