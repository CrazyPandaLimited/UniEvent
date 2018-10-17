#pragma once
#include "Loop.h"
#include "Request.h"

namespace panda { namespace unievent {

struct Work : Request, AllocatedObject<Work> {
    typedef panda::function<void(Work*)> work_fn;
    typedef panda::function<void(Work*)> after_work_fn;

    work_fn       work_event;
    after_work_fn after_work_event;

    Work (Loop* loop = Loop::default_loop()) {
        uvr.loop = _pex_(loop);
        _init(&uvr);
    }

    virtual void queue ();

protected:
    virtual void on_work       ();
    virtual void on_after_work ();

private:
    uv_work_t uvr;

    static void uvx_on_work       (uv_work_t* req);
    static void uvx_on_after_work (uv_work_t* req, int status);
};

}}
