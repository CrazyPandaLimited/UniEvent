#pragma once
#include "../Debug.h"
#include "BackendLoop.h"
#include <panda/string.h>
#include <panda/optional.h>

namespace panda { namespace unievent { namespace backend {

struct BackendRequest {
    BackendHandle* handle;

    BackendRequest (BackendHandle* handle) : handle(handle) { _ECTOR(); }

    virtual void destroy () noexcept = 0;
    virtual ~BackendRequest () { _EDTOR(); }
};

struct BackendHandle {
    static constexpr const size_t MIN_ALLOC_SIZE = 1024;

    uint64_t     id;
    BackendLoop* loop;

    BackendHandle (BackendLoop* loop) : id(++last_id), loop(loop) {}

    virtual bool active () const = 0;

    virtual void set_weak   () = 0;
    virtual void unset_weak () = 0;

    template <class Func>
    void ltry (Func&& f) { loop->ltry(f); }

    virtual void destroy () noexcept = 0;

    virtual ~BackendHandle () {}

    template <class T>
    static string buf_alloc (size_t size, T allocator) noexcept {
        if (size < MIN_ALLOC_SIZE) size = MIN_ALLOC_SIZE;
        return allocator->buf_alloc(size);
    }

private:
    static uint64_t last_id;

};

}}}
