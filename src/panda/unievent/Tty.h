#pragma once
#include "Stream.h"
#include "backend/TtyImpl.h"

namespace panda { namespace unievent {

struct ITtyListener     : IStreamListener     {};
struct ITtySelfListener : IStreamSelfListener {};

struct Tty : virtual Stream {
    using TtyImpl = backend::TtyImpl;
    using Mode       = TtyImpl::Mode;
    using WinSize    = TtyImpl::WinSize;

    static const HandleType TYPE;

    static void reset_mode ();

    Tty (fd_t fd, const LoopSP& loop = Loop::default_loop());

    const HandleType& type () const override;

    fd_t fd () const { return _fd; }

    excepted<net::SockAddr, ErrorCode> sockaddr () const override { return {}; }
    excepted<net::SockAddr, ErrorCode> peeraddr () const override { return {}; }

    virtual excepted<void, ErrorCode>    set_mode(Mode);
    virtual excepted<WinSize, ErrorCode> get_winsize ();

protected:
    StreamSP create_connection () override;
    void     on_reset          () override;

private:
    fd_t _fd;

    TtyImpl*    impl     () const { return static_cast<TtyImpl*>(BackendHandle::impl()); }
    HandleImpl* new_impl () override;
};
using TtySP = iptr<Tty>;

}}
