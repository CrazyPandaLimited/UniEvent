#pragma once

#include "Error.h"
#include <panda/function.h>

namespace panda { namespace unievent {

struct Resolver;
struct ResolveRequest;
struct AddrInfo;
using ResolveFunctionPlain = void(iptr<Resolver>, iptr<ResolveRequest>, iptr<AddrInfo> address, const CodeError* err);
using ResolveFunction      = function<ResolveFunctionPlain>;

}} // namespace panda::event
