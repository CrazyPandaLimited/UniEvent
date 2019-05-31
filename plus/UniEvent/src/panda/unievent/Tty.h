#pragma once
#include "Stream.h"
#include "backend/TtyImpl.h"

namespace panda { namespace unievent {

struct Tty : virtual Stream {
    using TtyImpl = backend::TtyImpl;
    using Mode       = TtyImpl::Mode;
    using WinSize    = TtyImpl::WinSize;

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
    TtyImpl*    impl     () const { return static_cast<TtyImpl*>(BackendHandle::impl()); }
    HandleImpl* new_impl () override;
};
using TtySP = iptr<Tty>;

}}
