#pragma once

#include "Handle.h"

namespace panda { namespace unievent {

struct Timer;
using TimerSP = iptr<Timer>;

// All the values are in milliseconds.
struct Timer : virtual Handle, AllocatedObject<Timer> {
    using timer_fptr = void(Timer* handle);
    using timer_fn = function<timer_fptr>;
    
    CallbackDispatcher<timer_fptr> timer_event;

    Timer (Loop* loop = Loop::default_loop()) {
        uv_timer_init(_pex_(loop), &uvh);
        _init(&uvh);
    }

    void once     (uint64_t initial) { start(0, initial); }
    void start    (uint64_t repeat)  { start(repeat, repeat); }
    void call_now ()                 { on_timer(); }

    virtual void     start  (uint64_t repeat, uint64_t initial);
    virtual void     stop   ();
    virtual void     again  ();
    virtual uint64_t repeat () const;
    virtual void     repeat (uint64_t repeat);

    void reset () override;

    static TimerSP once  (uint64_t initial, timer_fn cb, Loop* loop = Loop::default_loop());
    static TimerSP start (uint64_t repeat,  timer_fn cb, Loop* loop = Loop::default_loop());

protected:
    virtual void on_timer ();

private:
    uv_timer_t uvh;

    static void uvx_on_timer (uv_timer_t* handle);
};

}}
