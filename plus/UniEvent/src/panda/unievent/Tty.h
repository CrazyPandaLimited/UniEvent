#pragma once
#include "Stream.h"
#include "backend/BackendTty.h"

namespace panda { namespace unievent {

struct Tty : virtual Stream {
    using BackendTty = backend::BackendTty;
    using Mode       = BackendTty::Mode;
    using WinSize    = BackendTty::WinSize;

    static const HandleType TYPE;

    static void reset_mode ();

    Tty (fd_t fd, const LoopSP& loop = Loop::default_loop());

    const HandleType& type () const override;

    virtual void    set_mode    (Mode);
    virtual WinSize get_winsize ();

protected:
    fd_t fd;

    StreamSP create_connection () override;

private:
    BackendTty*    impl     () const { return static_cast<BackendTty*>(Handle::impl()); }
    BackendHandle* new_impl () override;
};
using TtySP = iptr<Tty>;

}}
