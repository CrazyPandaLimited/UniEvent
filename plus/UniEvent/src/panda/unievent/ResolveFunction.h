#pragma once

#include "Error.h"
#include <panda/function.h>

namespace panda { namespace unievent {

struct Resolver;
struct ResolveRequest;
struct BasicAddress;
using ResolveFunctionPlain = void(Resolver*, iptr<ResolveRequest>, iptr<BasicAddress> address, const CodeError* err);
using ResolveFunction      = function<ResolveFunctionPlain>;

}} // namespace panda::event
