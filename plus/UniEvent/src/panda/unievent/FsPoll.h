#pragma once
#include "Fs.h"
#include "Timer.h"

namespace panda { namespace unievent {

struct FsPoll : virtual Handle {
    using fs_poll_fptr = void(const FsPollSP&, const Fs::Stat& prev, const Fs::Stat& cur, const CodeError&);
    using fs_poll_fn   = function<fs_poll_fptr>;

    static const HandleType TYPE;

    CallbackDispatcher<fs_poll_fptr> event;

    FsPoll (const LoopSP& loop = Loop::default_loop());

    const HandleType& type () const override;

    const string& path () const { return _path; }

    bool active () const override { return timer->active(); }

    void set_weak   () override { timer->weak(true); }
    void unset_weak () override { timer->weak(false); }

    virtual void start (std::string_view path, unsigned int interval = 1000, const fs_poll_fn& callback = {});
    virtual void stop  ();

    void reset () override;
    void clear () override;

protected:
    virtual void on_fs_poll (const Fs::Stat& prev, const Fs::Stat& cur, const CodeError&);

private:
    TimerSP           timer;
    Fs::RequestSP     fsr;
    string            _path;
    weak_iptr<FsPoll> wself;
    Fs::Stat          prev;
    CodeError         prev_err;
    bool              fetched;

    void do_stat ();
};

}}
