#pragma once
#include "UVHandle.h"
#include <panda/unievent/backend/FsEventImpl.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVFsEvent : UVHandle<FsEventImpl, uv_fs_event_t> {
    UVFsEvent (UVLoop* loop, IFsEventListener* lst) : UVHandle<FsEventImpl, uv_fs_event_t>(loop, lst) {
        uvx_strict(uv_fs_event_init(loop->uvloop, &uvh));
    }

    void start (std::string_view path, unsigned flags) override {
        unsigned uv_flags = 0;
        if (flags & Flags::RECURSIVE) uv_flags |= UV_FS_EVENT_RECURSIVE;
        UE_NULL_TERMINATE(path, path_str);
        uvx_strict(uv_fs_event_start(&uvh, on_fs_event, path_str, flags));
    }

    void stop () override {
        uvx_strict(uv_fs_event_stop(&uvh));
    }

private:
    static void on_fs_event (uv_fs_event_t* p, const char* filename, int uv_events, int status) {
        auto sv = status ? std::string_view() : std::string_view(filename);
        int events = 0;
        if (uv_events & UV_RENAME) events |= Event::RENAME;
        if (uv_events & UV_CHANGE) events |= Event::CHANGE;
        get_handle<UVFsEvent*>(p)->handle_fs_event(sv, events, uvx_ce(status));
    }
};

}}}}
