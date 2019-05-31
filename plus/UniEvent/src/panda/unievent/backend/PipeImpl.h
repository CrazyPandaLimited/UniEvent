#pragma once
#include "StreamImpl.h"
#include <panda/optional.h>

namespace panda { namespace unievent { namespace backend {

struct PipeImpl : StreamImpl {
    PipeImpl (LoopImpl* loop, IStreamListener* lst) : StreamImpl(loop, lst) {}

    virtual void open (fd_t file) = 0;
    virtual void bind (std::string_view name) = 0;

    virtual CodeError connect (std::string_view name, ConnectRequestImpl* req) = 0;

    virtual optional<string> sockname () const = 0;
    virtual optional<string> peername () const = 0;

    virtual void pending_instances (int count) = 0;
};

}}}
