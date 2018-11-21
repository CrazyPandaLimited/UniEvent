// Forward declarations only
#pragma once

#include <panda/refcnt.h>
#include <panda/function.h>

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

struct StreamFilter;
using StreamFilterSP = iptr<StreamFilter>;

struct TCP;
using TCPSP = iptr<TCP>;

struct Pipe;
using PipeSP = iptr<Pipe>;

struct SimpleResolver;
using SimpleResolverSP = iptr<SimpleResolver>;

struct Resolver;
using ResolverSP = iptr<Resolver>;

struct Request;

struct ConnectRequest;
using ConnectRequestSP = iptr<ConnectRequest>;

struct TCPConnectRequest;
using TCPConnectRequestSP = iptr<TCPConnectRequest>;

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

struct CodeError;

struct TCPConnectAutoBuilder;

using ResolveFunctionPlain = void(SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err);
using ResolveFunction = function<ResolveFunctionPlain>;

}} // namespace panda::unievent
