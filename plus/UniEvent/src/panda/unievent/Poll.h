#pragma once
#include "Handle.h"
#include "backend/BackendPoll.h"

namespace panda { namespace unievent {

struct Poll : virtual BHandle, private backend::IPollListener {
    using poll_fptr = void(const PollSP& handle, int events, const CodeError& err);
    using poll_fn = panda::function<poll_fptr>;

    enum {
      READABLE = 1,
      WRITABLE = 2
    };

    struct Socket { sock_t val; };
    struct Fd     { int    val; };

    CallbackDispatcher<poll_fptr> event;

    Poll (Socket sock, const LoopSP& loop = Loop::default_loop(), Ownership ownership = Ownership::TRANSFER);
    Poll (Fd       fd, const LoopSP& loop = Loop::default_loop(), Ownership ownership = Ownership::TRANSFER);

    ~Poll () {
        _EDTOR();
    }

    const HandleType& type () const override;

    virtual void start (int events, poll_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;
    void clear () override;

    void call_now (int events, const CodeError& err) { on_poll(events, err); }

    optional<fh_t> fileno () const { return _impl ? impl()->fileno() : optional<fh_t>(); }

    static const HandleType TYPE;

protected:
    virtual void on_poll (int events, const CodeError& err);

private:
    void handle_poll (int events, const CodeError& err) override;

    backend::BackendPoll* impl () const { return static_cast<backend::BackendPoll*>(_impl); }
};

}}
