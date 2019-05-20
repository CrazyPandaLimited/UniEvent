#pragma once
#include "UVStream.h"
#include <panda/unievent/backend/BackendTty.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVTty : UVStream<BackendTty, uv_tty_t> {
    UVTty (UVLoop* loop, IStreamListener* lst, fd_t fd) : UVStream<BackendTty, uv_tty_t>(loop, lst) {
        uvx_strict(uv_tty_init(loop->uvloop, &uvh, fd, /*not used*/0));
    }

    void set_mode (Mode mode) override {
        uv_tty_mode_t uv_mode;
        switch (mode) {
            case Mode::STD : uv_mode = UV_TTY_MODE_NORMAL; break;
            case Mode::RAW : uv_mode = UV_TTY_MODE_RAW;    break;
            case Mode::IO  : uv_mode = UV_TTY_MODE_IO;     break;
        }
        uvx_strict(uv_tty_set_mode(&uvh, uv_mode));
    }

    WinSize get_winsize () override {
        WinSize ret;
        uvx_strict(uv_tty_get_winsize(&uvh, &ret.width, &ret.height));
        return ret;
    }
};

}}}}
