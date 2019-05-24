#pragma once
#include "../Error.h"
#include "../Debug.h"
#include "BackendLoop.h"

namespace panda { namespace unievent { namespace backend {

struct IWorkListener {
    virtual void handle_work       () = 0;
    virtual void handle_after_work (const CodeError&) = 0;
};

struct BackendWork {
    BackendLoop*   loop;
    IWorkListener* listener;

    BackendWork (BackendLoop* loop, IWorkListener* lst) : loop(loop), listener(lst) { _ECTOR(); }

    virtual void queue () = 0;

    void handle_work () noexcept {
        loop->ltry([&]{ listener->handle_work(); });
    }

    void handle_after_work (const CodeError& err) noexcept {
        loop->ltry([&]{ listener->handle_after_work(err); });
    }

    virtual bool destroy () noexcept = 0;

    virtual ~BackendWork () { _EDTOR(); }
};

}}}
