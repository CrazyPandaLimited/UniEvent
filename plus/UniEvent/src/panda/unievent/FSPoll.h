#pragma once
#include "Handle.h"

namespace panda { namespace unievent {

struct FSPoll : virtual Handle {
    using fs_poll_fptr = void(FSPoll* handle, const stat_t* prev, const stat_t* curr, const CodeError* err);
    using fs_poll_fn = function<fs_poll_fptr>;

    CallbackDispatcher<fs_poll_fptr> fs_poll_event;

    FSPoll (Loop* loop = Loop::default_loop()) {
        int err = uv_fs_poll_init(_pex_(loop), &uvh);
        if (err) throw new CodeError(err);
        _init(&uvh);
    }

    virtual void start (const char* path, unsigned int interval = 1000, fs_poll_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    size_t getpathlen () const {
        size_t len = 0;
        int err = uv_fs_poll_getpath((uv_fs_poll_t*)&uvh, nullptr, &len);
        if (err && err != UV_ENOBUFS) throw CodeError(err);
        return len - 1; // because libuv returns len with null-byte
    }

    void getpath (char* buf) const {
        size_t len = (size_t)-1;
        int err = uv_fs_poll_getpath((uv_fs_poll_t*)&uvh, buf, &len);
        if (err) throw CodeError(err);
    }

    panda::string getpath () const {
        auto len = getpathlen();
        panda::string str(len + 1); // need 1 byte more because libuv null-terminates string
        getpath(str.buf());
        str.length(len);
        return str;
    }

    void call_on_fs_poll (const stat_t* prev, const stat_t* curr, const CodeError* err) { on_fs_poll(prev, curr, err); }

protected:
    virtual void on_fs_poll (const stat_t* prev, const stat_t* curr, const CodeError* err);

private:
    uv_fs_poll_t uvh;

    static void uvx_on_fs_poll (uv_fs_poll_t* handle, int errcode, const stat_t* prev, const stat_t* curr);
};

using FSPollSP = iptr<FSPoll>;

}}
