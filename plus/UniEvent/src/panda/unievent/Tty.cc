#include "Tty.h"
#include "uv.h"
using namespace panda::unievent;

const HandleType Tty::TYPE("tty");

void Tty::reset_mode () {
    uv_tty_reset_mode();
}

Tty::Tty (fd_t fd, const LoopSP& loop) : fd(fd) {
    _init(loop, loop->impl()->new_tty(this, fd));
}

const HandleType& Tty::type () const {
    return TYPE;
}

backend::HandleImpl* Tty::new_impl () {
    return loop()->impl()->new_tty(this, fd);
}

StreamSP Tty::create_connection () {
    return new Tty(fd, loop());
}

void Tty::set_mode (Mode mode) {
    impl()->set_mode(mode);
}

Tty::WinSize Tty::get_winsize () {
    return impl()->get_winsize();
}
