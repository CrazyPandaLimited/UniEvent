#pragma once

#include <panda/unievent/Error.h>
#include <panda/function.h>

namespace panda { namespace unievent {

class AbstractResolver;
class ResolveRequest;
class BasicAddress;
using ResolveFunctionPlain = void(AbstractResolver*, iptr<ResolveRequest>, iptr<BasicAddress> address, const CodeError* err);
using ResolveFunction      = function<ResolveFunctionPlain>;

}} // namespace panda::event
