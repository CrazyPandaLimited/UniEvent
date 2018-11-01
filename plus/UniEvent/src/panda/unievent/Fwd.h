// Forward declarations only
#pragma once

#include <panda/refcnt.h>

namespace panda { namespace unievent {

struct Handle;
using HandleSP = iptr<Handle>;

struct Loop;
using LoopSP = iptr<Loop>;

struct Poll;
using PollSP = iptr<Poll>;

struct FSPoll;
using FSPollSP = iptr<FSPoll>;

struct Timer;
using TimerSP = iptr<Timer>;

struct Prepare;
using PrepareSP = iptr<Prepare>;

struct Stream;
using StreamSP = iptr<Stream>;

struct TCP;
using TCPSP = iptr<TCP>;

struct Resolver;
using ResolverSP = iptr<Resolver>;

struct CachedResolver;
using CachedResolverSP = iptr<CachedResolver>;

struct ConnectRequest;
using ConnectRequestSP = iptr<ConnectRequest>;

struct ResolveRequest;
using ResolveRequestSP = iptr<ResolveRequest>;

struct AddrInfoHints;
using AddrInfoHintsSP = iptr<AddrInfoHints>;

struct AddrInfo;
using AddrInfoSP = iptr<AddrInfo>;

struct AddressRotator;
using AddressRotatorSP = iptr<AddressRotator>;

struct CachedAddress;
using CachedAddressSP = iptr<CachedAddress>;

struct Socks;
using SocksSP = iptr<Socks>;

struct AresTask;
using AresTaskSP = iptr<AresTask>;

}} // namespace panda::unievent
