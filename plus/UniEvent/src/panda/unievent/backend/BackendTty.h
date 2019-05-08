#pragma once
#include "BackendStream.h"

namespace panda { namespace unievent { namespace backend {

struct BackendTty : BackendStream {
    enum class Mode { STD = 0, RAW, IO };

    struct WinSize {
        int width;
        int height;
    };

    BackendTty (BackendLoop* loop, IStreamListener* lst) : BackendStream(loop, lst) {}

    virtual void    set_mode    (Mode) = 0;
    virtual WinSize get_winsize ()     = 0;
};

}}}
