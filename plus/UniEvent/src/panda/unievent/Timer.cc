#include "Timer.h"
using namespace panda::unievent;

const HandleType Timer::TYPE("timer");

const HandleType& Timer::type () const {
    return TYPE;
}

void Timer::start (uint64_t repeat, uint64_t initial) {
    //loop()->update_time();
    impl()->start(repeat, initial);
}

void Timer::stop () {
    impl()->stop();
}

void Timer::again () {
    //loop()->update_time();
    impl()->again();
}

uint64_t Timer::repeat () const {
    return impl()->repeat();
}

void Timer::repeat (uint64_t repeat) {
    //loop()->update_time();
    impl()->repeat(repeat);
}

TimerSP Timer::once (uint64_t initial, timer_fn cb, Loop* loop) {
    TimerSP timer = new Timer(loop);
    timer->timer_event.add(cb);
    timer->once(initial);
    return timer;
}

TimerSP Timer::start (uint64_t repeat, timer_fn cb, Loop* loop) {
    //loop->update_time();
    TimerSP timer = new Timer(loop);
    timer->timer_event.add(cb);
    timer->start(repeat);
    return timer;
}

void Timer::on_timer () {
    timer_event(this);
}

void Timer::handle_timer () {
    on_timer();
}
