#pragma once
#include <panda/unievent/Handle.h>

namespace panda { namespace unievent {

class FSEvent : public virtual Handle {
public:
    using fs_event_fptr = void(FSEvent* handle, const char* filename, int events);
    using fs_event_fn = function<fs_event_fptr>;
    
    CallbackDispatcher<fs_event_fptr> fs_event;

    enum fs_event {
        RENAME = UV_RENAME,
        CHANGE = UV_CHANGE
    };

    enum fs_event_flags {
        WATCH_ENTRY = UV_FS_EVENT_WATCH_ENTRY,
        STAT        = UV_FS_EVENT_STAT,
        RECURSIVE   = UV_FS_EVENT_RECURSIVE
    };

    FSEvent (Loop* loop = Loop::default_loop()) {
        int err = uv_fs_event_init(_pex_(loop), &uvh);
        if (err) throw FSEventError(err);
        _init(&uvh);
    }

    virtual void start (const char* path, int flags = 0, fs_event_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    size_t getpathlen () const {
        size_t len = 0;
        int err = uv_fs_event_getpath((uv_fs_event_t*)&uvh, nullptr, &len);
        if (err && err != UV_ENOBUFS) throw FSEventError(err);
        return len-1; // there's a bug in libuv and it returns length with terminating null-byte included
    }

    void getpath (char* buf) const {
        size_t len = (size_t)-1;
        int err = uv_fs_event_getpath((uv_fs_event_t*)&uvh, buf, &len);
        if (err) throw FSEventError(err);
    }

    panda::string getpath () const {
        auto len = getpathlen();
        panda::string str(len + 1); // need space for null-byte
        getpath(str.buf());
        str.length(len);
        return str;
    }

    void call_on_fs_event (const char* filename, int events) { on_fs_event(filename, events); }

protected:
    virtual void on_fs_event (const char* filename, int events);

private:
    uv_fs_event_t uvh;

    static void uvx_on_fs_event (uv_fs_event_t* handle, const char* filename, int events, int status);
};

}}
