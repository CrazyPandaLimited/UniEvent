#pragma once
#include "Error.h"
#include <string.h>
#include <panda/CallbackDispatcher.h>

namespace panda { namespace unievent {

typedef uv_thread_t thread_t;

struct Thread {
    typedef panda::function <void(Thread* handle, void* arg)> entry_fn;
    
    entry_fn create_callback;

    static thread_t self () {
        if (!_thread_self) _thread_self = static_cast<thread_t>(uv_thread_self());
        return _thread_self;
    }

    Thread () {
        ctx.myself = this;
        memset(&handle, 0, sizeof(handle)); // suppress warning 'member xxx was not initialized in constructor'
    }

    virtual void create (void* arg = nullptr);
    virtual void join   ();

protected:
    virtual void on_create (void* arg);

private:
    struct thread_ctx_t {
        void*   arg;
        Thread* myself;
    };

    thread_t  handle;
    thread_ctx_t ctx;

    static thread_local thread_t _thread_self;

    static void uvx_on_create (void* arg);
};

}}
