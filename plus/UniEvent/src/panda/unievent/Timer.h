#pragma once
#include "Handle.h"
#include "backend/BackendTimer.h"

namespace panda { namespace unievent {

// All the values are in milliseconds.
struct Timer : virtual BHandle, private backend::ITimerListener {
    using timer_fptr = void(const TimerSP& handle);
    using timer_fn = function<timer_fptr>;

    static const HandleType TYPE;

    CallbackDispatcher<timer_fptr> event;

    Timer (const LoopSP& loop = Loop::default_loop()) {
        _init(loop, loop->impl()->new_timer(this));
    }

    const HandleType& type () const override;

    void once     (uint64_t initial) { start(0, initial); }
    void start    (uint64_t repeat)  { start(repeat, repeat); }
    void call_now ()                 { on_timer(); }

    virtual void     start  (uint64_t repeat, uint64_t initial);
    virtual void     stop   ();
    virtual void     again  ();
    virtual uint64_t repeat () const;
    virtual void     repeat (uint64_t repeat);

    void reset () override;
    void clear () override;

    static TimerSP once  (uint64_t initial, timer_fn cb, Loop* loop = Loop::default_loop());
    static TimerSP start (uint64_t repeat,  timer_fn cb, Loop* loop = Loop::default_loop());

protected:
    virtual void on_timer ();

private:
    void handle_timer () override;

    backend::BackendTimer* impl () const { return static_cast<backend::BackendTimer*>(_impl); }
};

}}
