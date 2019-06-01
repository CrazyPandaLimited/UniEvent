#pragma once
#include "BackendHandle.h"
#include "backend/FsEventImpl.h"

namespace panda { namespace unievent {

struct FsEvent : virtual BackendHandle, private backend::IFsEventListener {
    using FsEventImpl   = backend::FsEventImpl;
    using Event         = FsEventImpl::Event;
    using Flags         = FsEventImpl::Flags;
    using fs_event_fptr = void(const FsEventSP&, const std::string_view& file, int events, const CodeError&);
    using fs_event_fn   = function<fs_event_fptr>;
    
    CallbackDispatcher<fs_event_fptr> event;

    FsEvent (const LoopSP& loop = Loop::default_loop()) {
        _init(loop, loop->impl()->new_fs_event(this));
    }

    const HandleType& type () const override;

    const string& path () const { return _path; }

    virtual void start (const std::string_view& path, int flags = 0, fs_event_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;
    void clear () override;

    static const HandleType TYPE;

protected:
    virtual void on_fs_event (const std::string_view& file, int events, const CodeError&);

private:
    string _path;

    void handle_fs_event (const std::string_view& file, int events, const CodeError&) override;

    FsEventImpl* impl () const { return static_cast<FsEventImpl*>(_impl); }
};

}}
