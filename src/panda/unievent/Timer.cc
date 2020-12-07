#include "Timer.h"

namespace panda { namespace unievent {

const HandleType Timer::TYPE("timer");

const HandleType& Timer::type () const {
    return TYPE;
}

void Timer::start (uint64_t repeat, uint64_t initial) {
    loop()->update_time();
    impl()->start(repeat, initial);
}

void Timer::stop () {
    impl()->stop();
}

excepted<void, ErrorCode> Timer::again () {
    loop()->update_time();
    return make_excepted(impl()->again());
}

uint64_t Timer::repeat () const {
    return impl()->repeat();
}

void Timer::repeat (uint64_t repeat) {
    impl()->repeat(repeat);
}

TimerSP Timer::once (uint64_t initial, timer_fn cb, const LoopSP& loop) {
    TimerSP timer = new Timer(loop);
    timer->event.add(cb);
    timer->once(initial);
    return timer;
}

TimerSP Timer::start (uint64_t repeat, timer_fn cb, const LoopSP& loop) {
    TimerSP timer = new Timer(loop);
    timer->event.add(cb);
    timer->start(repeat);
    return timer;
}

void Timer::reset () {
    impl()->stop();
}

void Timer::clear () {
    impl()->stop();
    weak(false);
    _listener = nullptr;
    event.remove_all();
}

void Timer::handle_timer () {
    TimerSP self = this;
    panda_log_debug("on timer " << loop()->impl());
    event(self);
    if (_listener) _listener->on_timer(self);
}

}}
