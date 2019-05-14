#pragma once
#include "BackendHandle.h"

namespace panda { namespace unievent { namespace backend {

struct IFsPollListener {
    virtual void handle_fs_poll (const Stat& prev, const Stat& curr, const CodeError& err) = 0;
};

struct BackendFsPoll : BackendHandle {
    BackendFsPoll (BackendLoop* loop, IFsPollListener* lst) : BackendHandle(loop), listener(lst) {}

    virtual void start (std::string_view path, unsigned int interval) = 0;
    virtual void stop  () = 0;

    virtual panda::string path () const = 0;

    void handle_fs_poll (const Stat& prev, const Stat& cur, const CodeError& err) noexcept {
        ltry([&]{ listener->handle_fs_poll(prev, cur, err); });
    }

    IFsPollListener* listener;
};

}}}
