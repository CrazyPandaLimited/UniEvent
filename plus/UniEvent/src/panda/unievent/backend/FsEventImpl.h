#pragma once
#include "HandleImpl.h"

namespace panda { namespace unievent { namespace backend {

struct IFsEventListener {
    virtual void handle_fs_event (const std::string_view& file, int events, const CodeError&) = 0;
};

struct FsEventImpl : HandleImpl {
    struct Event {
        static constexpr int RENAME = 1;
        static constexpr int CHANGE = 2;
    };

    struct Flags {
        static constexpr int RECURSIVE = 1;
    };

    FsEventImpl (LoopImpl* loop, IFsEventListener* lst) : HandleImpl(loop), listener(lst) {}

    virtual void start (std::string_view path, unsigned flags) = 0;
    virtual void stop  () = 0;

    void handle_fs_event (const std::string_view& file, int events, const CodeError& err) noexcept {
        ltry([&]{ listener->handle_fs_event(file, events, err); });
    }

    IFsEventListener* listener;
};

}}}
