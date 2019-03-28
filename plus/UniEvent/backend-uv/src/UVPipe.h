#pragma once
#include "inc.h"
#include "UVStream.h"
#include <panda/unievent/backend/BackendPipe.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPipe : UVStream<BackendPipe> {
    UVPipe (uv_loop_t* loop, IStreamListener* lst, bool ipc) : UVStream<BackendPipe>(lst) {
        uvx_strict(uv_pipe_init(loop, &uvh, ipc));
        _init(&uvh);
    }

    virtual void bind (std::string_view name) override {
        UE_NULL_TERMINATE(name, name_str);
        uvx_strict(uv_pipe_bind(&uvh, name_str));
    }

private:
    uv_pipe_t uvh;
};

}}}}
