#pragma once

namespace panda { namespace unievent { namespace backend {

struct BackendLoop;
struct BackendHandle;

struct BackendTimer;
struct ITimerListener;

struct BackendPrepare;
struct IPrepareListener;

struct BackendCheck;
struct ICheckListener;

struct BackendIdle;
struct IIdleListener;

struct BackendAsync;
struct IAsyncListener;

struct BackendSignal;
struct ISignalListener;

struct BackendPoll;
struct IPollListener;

struct BackendTick;
struct ITickListener;

struct BackendUdp;
struct IUdpListener;
struct ISendListener;
struct BackendSendRequest;

}}};
