#pragma once
#include <panda/unievent/Handle.h>

namespace panda { namespace unievent {

class Poll : public virtual Handle {
public:
    using poll_fptr = void(Poll* handle, int events, const CodeError& err);
    using poll_fn = panda::function<poll_fptr>;
    
    CallbackDispatcher<poll_fptr> poll_event;

    enum poll_event_t {
      READABLE   = UV_READABLE,
      WRITABLE   = UV_WRITABLE,
      DISCONNECT = UV_DISCONNECT 
    };

    Poll (int fd, sock_t socket, Loop* loop = Loop::default_loop()) {
        int err;
        if (fd >= 0) err = uv_poll_init(_pex_(loop), &uvh, fd);
        else err = uv_poll_init_socket(_pex_(loop), &uvh, socket);
        if (err) throw CodeError(err);
        _init(&uvh);
    }

    virtual void start (int events, poll_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_on_poll (int events, const CodeError& err) { on_poll(events, err); }

protected:
    virtual void on_poll (int events, const CodeError& err);

private:
    uv_poll_t uvh;

    static void uvx_on_poll (uv_poll_t* handle, int status, int events);
};

}}
