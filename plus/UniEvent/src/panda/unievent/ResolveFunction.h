#pragma once

#include <panda/unievent/Error.h>
#include <panda/function.h>

namespace panda { namespace unievent {

using ResolveFunctionPlain = void(addrinfo* res, const ResolveError& err, bool from_cache);
using ResolveFunction      = function<ResolveFunctionPlain>;

}} // namespace panda::event
