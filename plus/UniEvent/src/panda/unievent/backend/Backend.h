#pragma once
#include "BackendLoop.h"
#include <panda/string.h>

namespace panda { namespace unievent { namespace backend {

struct Backend {

    panda::string type () const { return _type; }

    virtual BackendLoop* new_loop (Loop* frontend, BackendLoop::Type type) = 0;

    virtual ~Backend () {}

protected:
    Backend (std::string_view type) : _type(type) {}

private:
    panda::string _type;
};

}}}
