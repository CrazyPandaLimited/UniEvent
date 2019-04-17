#pragma once
#include "BackendStream.h"
#include <panda/optional.h>

namespace panda { namespace unievent { namespace backend {

struct BackendPipe : BackendStream {
    BackendPipe (BackendLoop* loop, IStreamListener* lst) : BackendStream(loop, lst) {}

    virtual void open (file_t file) = 0;
    virtual void bind (std::string_view name) = 0;

    virtual CodeError connect (std::string_view name, BackendConnectRequest* req) = 0;

    virtual optional<string> sockname () const = 0;
    virtual optional<string> peername () const = 0;

    virtual void pending_instances (int count) = 0;
};

}}}
