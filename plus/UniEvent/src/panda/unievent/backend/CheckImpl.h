#pragma once
#include "HandleImpl.h"

namespace panda { namespace unievent { namespace backend {

struct ICheckListener {
    virtual void handle_check () = 0;
};

struct CheckImpl : HandleImpl {
    CheckImpl (LoopImpl* loop, ICheckListener* lst) : HandleImpl(loop), listener(lst) {}

    virtual void start () = 0;
    virtual void stop  () = 0;

    void handle_check () noexcept {
        ltry([&]{ listener->handle_check(); });
    }

    ICheckListener* listener;
};

}}}
