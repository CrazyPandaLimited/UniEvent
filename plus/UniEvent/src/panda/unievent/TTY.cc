#include "TTY.h"
using namespace panda::unievent;

StreamSP TTY::on_create_connection () {
    return new TTY(fd, readable(), loop());
}

void TTY::set_mode (int mode) {
    int err = uv_tty_set_mode(&uvh, mode);
    if (err) throw CodeError(err);
}

TTY::WinSize TTY::get_winsize () {
    WinSize ret;
    int err = uv_tty_get_winsize(&uvh, &ret.width, &ret.height);
    if (err) throw CodeError(err);
    return ret;
}

void TTY::on_handle_reinit () {
    int err = uv_tty_init(uvh.loop, &uvh, fd, readable());
    if (err) throw CodeError(err);
    Stream::on_handle_reinit();
}
