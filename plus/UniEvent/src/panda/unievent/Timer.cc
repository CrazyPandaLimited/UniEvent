#include <panda/unievent/Timer.h>
using namespace panda::unievent;

void Timer::uvx_on_timer (uv_timer_t* handle) {
    Timer* h = hcast<Timer*>(handle);
    h->call_now();
}

void Timer::start (uint64_t repeat, uint64_t initial) {
    loop()->update_time();
    uv_timer_start(&uvh, uvx_on_timer, initial, repeat);
}

void Timer::stop  () { uv_timer_stop(&uvh); }
void Timer::reset () { uv_timer_stop(&uvh); }


void Timer::again () {
    loop()->update_time();
    int err = uv_timer_again(&uvh);
    if (err) throw CodeError(err);
}

uint64_t Timer::repeat () const { return uv_timer_get_repeat(&uvh); }

void Timer::repeat (uint64_t repeat) {
    loop()->update_time();
    uv_timer_set_repeat(&uvh, repeat);
}

TimerSP Timer::once(uint64_t initial, timer_fn cb, Loop* loop) {
    TimerSP timer = new Timer(loop);
    timer->timer_event.add(cb);
    timer->once(initial);
    return timer;
}

TimerSP Timer::start(uint64_t repeat, timer_fn cb, Loop* loop) {
    loop->update_time();
    TimerSP timer = new Timer(loop);
    timer->timer_event.add(cb);
    timer->start(repeat);
    return timer;
}

void Timer::on_timer () {
    if (timer_event.has_listeners()) timer_event(this);
    else throw ImplRequiredError("Timer::on_timer");
}
