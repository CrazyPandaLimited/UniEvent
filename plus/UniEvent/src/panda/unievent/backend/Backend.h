#pragma once
#include "LoopImpl.h"
#include <panda/string.h>

namespace panda { namespace unievent { namespace backend {

struct Backend {

    panda::string name () const { return _name; }

    virtual LoopImpl* new_loop (LoopImpl::Type type) = 0;

    virtual ~Backend () {}

protected:
    Backend (std::string_view name) : _name(name) {}

private:
    panda::string _name;
};

}}}
