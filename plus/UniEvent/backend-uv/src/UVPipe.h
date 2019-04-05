#pragma once
#include "inc.h"
#include "UVStream.h"
#include <panda/unievent/backend/BackendPipe.h>

namespace panda { namespace unievent { namespace backend { namespace uv {

struct UVPipe : UVStream<BackendPipe, uv_pipe_t> {
    UVPipe (uv_loop_t* loop, IStreamListener* lst, bool ipc) : UVStream<BackendPipe, uv_pipe_t>(lst) {
        uvx_strict(uv_pipe_init(loop, &uvh, ipc));
    }

    virtual void bind (std::string_view name) override {
        UE_NULL_TERMINATE(name, name_str);
        uvx_strict(uv_pipe_bind(&uvh, name_str));
    }

    virtual CodeError connect (std::string_view name, BackendConnectRequest* _req) override {
        UE_NULL_TERMINATE(name, name_str);
        auto req = static_cast<UVConnectRequest*>(_req);
        uv_pipe_connect(&req->uvr, &uvh, name_str, on_connect);
        req->active = true;
        return {};
    }
};

}}}}
