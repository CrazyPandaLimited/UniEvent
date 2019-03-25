#pragma once
#include "BackendLoop.h"
#include <panda/optional.h>

namespace panda { namespace unievent { namespace backend {

struct BackendRequest {
    virtual BackendHandle* handle () const noexcept = 0;

    template <class Func>
    void ltry (Func&& f) {
        try { f(); }
        catch (...) { capture_exception(); }
    }

    void capture_exception ();

    virtual void destroy () noexcept = 0;
    virtual ~BackendRequest () {}
};

struct BackendHandle {
    static constexpr const size_t MIN_ALLOC_SIZE = 1024;

    virtual BackendLoop* loop () const noexcept = 0;

    virtual bool active () const = 0;

    virtual void set_weak   () = 0;
    virtual void unset_weak () = 0;

    virtual optional<fd_t> fileno () const = 0;

    virtual int  recv_buffer_size () const    = 0;
    virtual void recv_buffer_size (int value) = 0;
    virtual int  send_buffer_size () const    = 0;
    virtual void send_buffer_size (int value) = 0;

    template <class Func>
    void ltry (Func&& f) {
        try { f(); }
        catch (...) { capture_exception(); }
    }

    void capture_exception ();

    virtual void destroy () noexcept = 0;

    virtual ~BackendHandle () {}
};

}}}
