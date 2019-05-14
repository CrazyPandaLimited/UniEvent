#pragma once
#include "Handle.h"
#include "backend/BackendFsPoll.h"

namespace panda { namespace unievent {

struct FsPoll : virtual Handle, private backend::IFsPollListener {
    using fs_poll_fptr = void(const FsPollSP&, const Stat& prev, const Stat& cur, const CodeError&);
    using fs_poll_fn = function<fs_poll_fptr>;

    static const HandleType TYPE;

    CallbackDispatcher<fs_poll_fptr> event;

    FsPoll (const LoopSP& loop = Loop::default_loop()) {
        _init(loop, loop->impl()->new_fs_poll(this));
    }

    const HandleType& type () const override;

    virtual void start (std::string_view path, unsigned int interval = 1000, const fs_poll_fn& callback = {});
    virtual void stop  ();

    void reset () override;
    void clear () override;

    panda::string path () const;

protected:
    virtual void on_fs_poll (const Stat& prev, const Stat& cur, const CodeError&);

private:
    void handle_fs_poll (const Stat&, const Stat&, const CodeError&) override;

    backend::BackendFsPoll* impl () const { return static_cast<backend::BackendFsPoll*>(_impl); }
};

}}
