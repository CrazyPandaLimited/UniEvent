#pragma once
#include "Handle.h"
#include "backend/BackendPoll.h"

namespace panda { namespace unievent {

struct Poll : virtual Handle {
    using poll_fptr = void(Poll* handle, int events, const CodeError* err);
    using poll_fn = panda::function<poll_fptr>;

    enum {
      READABLE = 1,
      WRITABLE = 2
    };

    struct Socket { sock_t val; };
    struct Fd     { int    val; };

    CallbackDispatcher<poll_fptr> poll_event;

    Poll (Socket sock, Loop* loop = Loop::default_loop()) {
        _ECTOR();
        _init(loop->impl()->new_poll_sock(this, sock.val));
    }

    Poll (Fd fd, Loop* loop = Loop::default_loop()) {
        _ECTOR();
        _init(loop->impl()->new_poll_fd(this, fd.val));
    }

    ~Poll () {
        _EDTOR();
    }

    virtual void start (int events, poll_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_now (int events, const CodeError* err) { on_poll(events, err); }

protected:
    virtual void on_poll (int events, const CodeError* err);

    backend::BackendPoll* impl () const { return static_cast<backend::BackendPoll*>(Handle::impl()); }
};

}}
