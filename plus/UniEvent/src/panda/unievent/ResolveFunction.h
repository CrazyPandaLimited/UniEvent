#pragma once

#include <panda/unievent/Error.h>
#include <panda/function.h>

namespace panda { namespace unievent {

class Resolver;
class ResolveRequest;
class BasicAddress;
using ResolveFunctionPlain = void(Resolver*, iptr<ResolveRequest>, iptr<BasicAddress> address, const CodeError* err);
using ResolveFunction      = function<ResolveFunctionPlain>;

}} // namespace panda::event
