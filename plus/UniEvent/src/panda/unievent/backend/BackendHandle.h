#pragma once
#include "../Debug.h"
#include "../Error.h"
#include "BackendLoop.h"
#include <panda/string.h>
#include <panda/optional.h>

namespace panda { namespace unievent { namespace backend {

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

struct IRequestListener {
    virtual void handle_event (const CodeError&) = 0;
};

struct BackendRequest {
    BackendHandle*    handle;
    IRequestListener* listener;

    BackendRequest (BackendHandle* h, IRequestListener* l) : handle(h), listener(l) { _ECTOR(); }

    void handle_event (const CodeError& err) noexcept {
        handle->loop->ltry([&]{ listener->handle_event(err); });
    }

    virtual void destroy () noexcept = 0;
    virtual ~BackendRequest () { _EDTOR(); }
};

}}}
