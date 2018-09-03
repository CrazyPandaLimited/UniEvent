#pragma once

#include <panda/unievent/Error.h>
#include <panda/function.h>

namespace panda { namespace unievent {

using ResolveFunctionPlain = void(addrinfo* res, const CodeError& err, bool from_cache);
using ResolveFunction      = function<ResolveFunctionPlain>;

}} // namespace panda::event
