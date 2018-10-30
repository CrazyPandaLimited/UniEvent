#pragma once
#include "Error.h"
#include <panda/function.h>

namespace panda { namespace unievent {

struct AbstractResolver;
struct ResolveRequest;
struct BasicAddress;
using ResolveFunctionPlain = void(AbstractResolver*, iptr<ResolveRequest>, iptr<BasicAddress> address, const CodeError* err);
using ResolveFunction      = function<ResolveFunctionPlain>;

}} // namespace panda::event
