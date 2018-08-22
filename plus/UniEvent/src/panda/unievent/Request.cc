#include <panda/unievent/Request.h>
#include <panda/unievent/Handle.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/Timer.h>

namespace panda { namespace unievent {

ConnectRequest::~ConnectRequest() {
    _ETRACETHIS("dtor %p", &uvr);
    release_timer();
}

void ConnectRequest::set_timer(Timer* timer) { 
    timer_ = timer; 
    timer_->retain();
    _EDEBUGTHIS("set timer %p", timer_);
}

void ConnectRequest::release_timer() { 
    _EDEBUGTHIS("%p", timer_);
    if(timer_) {
        timer_->stop();
        timer_->release();
        timer_ = nullptr;
    }
}

}}

