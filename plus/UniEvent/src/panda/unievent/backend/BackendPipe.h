#pragma once
#include "BackendStream.h"

namespace panda { namespace unievent { namespace backend {

struct BackendPipe : BackendStream {
    BackendPipe (IStreamListener* l) : BackendStream(l) {}

    virtual void bind (std::string_view name) = 0;
};

}}}
