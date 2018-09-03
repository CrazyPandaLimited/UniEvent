#pragma once
#include <panda/unievent/Stream.h>

namespace panda { namespace unievent {

class TTY : public virtual Stream {
public:
    struct WinSize {
        int width;
        int height;
    };

    static void reset_mode () { uv_tty_reset_mode(); }

    TTY (file_t fd, bool readable = false, Loop* loop = Loop::default_loop()) : fd(fd) {
        int err = uv_tty_init(_pex_(loop), &uvh, fd, readable);
        if (err) throw CodeError(err);
        _init(&uvh);
    }

    virtual void    set_mode    (int mode);
    virtual WinSize get_winsize ();

protected:
    void on_handle_reinit () override;

private:
    uv_tty_t uvh;
    file_t   fd;
};

}}
