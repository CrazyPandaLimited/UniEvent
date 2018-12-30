#pragma once
#include "BackendLoop.h"
#include <panda/string.h>

namespace panda { namespace unievent { namespace backend {

struct Backend {

    panda::string name () const { return _name; }

    virtual BackendLoop* new_loop (Loop* frontend, BackendLoop::Type type) = 0;

    virtual ~Backend () {}

protected:
    Backend (std::string_view name) : _name(name) {}

private:
    panda::string _name;
};

}}}
