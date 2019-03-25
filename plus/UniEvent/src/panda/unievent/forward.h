#pragma once
#include <panda/refcnt.h>
//#include <panda/function.h>

// Forward declarations only

namespace panda { namespace unievent {

struct Loop;
using LoopSP = iptr<Loop>;

struct Handle;
using HandleSP = iptr<Handle>;

struct Prepare;
using PrepareSP = iptr<Prepare>;

struct Check;
using CheckSP = iptr<Check>;

struct Idle;
using IdleSP = iptr<Idle>;

struct Timer;
using TimerSP = iptr<Timer>;

struct Async;
using AsyncSP = iptr<Async>;

struct Signal;
using SignalSP = iptr<Signal>;

struct Poll;
using PollSP = iptr<Poll>;

struct Resolver;
using ResolverSP = iptr<Resolver>;

struct Request;
using RequestSP = iptr<Request>;

struct Udp;
using UdpSP = iptr<Udp>;

//struct FSPoll;
//using FSPollSP = iptr<FSPoll>;
//
//struct Stream;
//using StreamSP = iptr<Stream>;
//
//struct StreamFilter;
//using StreamFilterSP = iptr<StreamFilter>;
//
//struct TCP;
//using TCPSP = iptr<TCP>;
//
//struct Pipe;
//using PipeSP = iptr<Pipe>;
//
//struct ConnectRequest;
//using ConnectRequestSP = iptr<ConnectRequest>;
//
//struct TCPConnectRequest;
//using TCPConnectRequestSP = iptr<TCPConnectRequest>;
//
//struct AddressRotator;
//using AddressRotatorSP = iptr<AddressRotator>;
//
//struct Socks;
//using SocksSP = iptr<Socks>;
//
//struct CodeError;
//
//struct TCPConnectAutoBuilder;

}}