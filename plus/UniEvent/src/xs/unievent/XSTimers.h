#pragma once
#include <panda/string_map.h>
#include <panda/unievent/Timer.h>

namespace xs { namespace unievent {

class XSTimers {
private:
    panda::string_map<string, XSTimer*> timers;
public:
    XSTimers () {
    }

//    void timer_callback (timer_cb cb) { on_timer_cb = cb; }
//
//    void once  (uint64_t initial) { start(0, initial); }
//    void start (uint64_t repeat)  { start(repeat, repeat); }
//
//    virtual void start (uint64_t repeat, uint64_t initial) {
//        uv_timer_start(&uvh, uvx_on_timer, initial, repeat);
//    }
//
//    virtual void stop () { uv_timer_stop(&uvh); }
//
//    virtual void again () {
//        if (uv_timer_again(&uvh)) throw CodeError(uvh.loop);
//    }
//
//    virtual uint64_t repeat () const          { return uv_timer_get_repeat(&uvh); }
//    virtual void     repeat (uint64_t repeat) { uv_timer_set_repeat(&uvh, repeat); }
};

}}
