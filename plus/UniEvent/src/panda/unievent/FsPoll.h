#pragma once
#include "Fs.h"
#include "Timer.h"

namespace panda { namespace unievent {

struct IFsPollListener {
    virtual void on_fs_poll (const FsPollSP&, const Fs::FStat& prev, const Fs::FStat& cur, const CodeError&) = 0;
};

struct IFsPollSelfListener : IFsPollListener {
    virtual void on_fs_poll (const Fs::FStat& prev, const Fs::FStat& cur, const CodeError&) = 0;
    void on_fs_poll (const FsPollSP&, const Fs::FStat& prev, const Fs::FStat& cur, const CodeError& err) override { on_fs_poll(prev, cur, err); }
};

struct FsPoll : virtual Handle {
    using fs_poll_fptr = void(const FsPollSP&, const Fs::FStat& prev, const Fs::FStat& cur, const CodeError&);
    using fs_poll_fn   = function<fs_poll_fptr>;

    static const HandleType TYPE;

    CallbackDispatcher<fs_poll_fptr> event;

    FsPoll (const LoopSP& loop = Loop::default_loop());

    const HandleType& type () const override;

    IFsPollListener* event_listener () const             { return _listener; }
    void             event_listener (IFsPollListener* l) { _listener = l; }

    const string& path () const { return _path; }

    bool active () const override { return timer->active(); }

    void set_weak   () override { timer->weak(true); }
    void unset_weak () override { timer->weak(false); }

    virtual void start (string_view path, unsigned int interval = 1000, const fs_poll_fn& callback = {});
    virtual void stop  ();

    void reset () override;
    void clear () override;

private:
    TimerSP           timer;
    Fs::RequestSP     fsr;
    string            _path;
    weak_iptr<FsPoll> wself;
    Fs::FStat         prev;
    CodeError         prev_err;
    bool              fetched;
    IFsPollListener*  _listener;

    void do_stat ();
    void notify  (const Fs::FStat&, const Fs::FStat&, const CodeError&);
};

}}
