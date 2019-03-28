#pragma once

namespace panda { namespace unievent { namespace backend {

struct BackendLoop;
struct BackendHandle;

struct BackendPrepare;
struct IPrepareListener;

struct BackendCheck;
struct ICheckListener;

struct BackendIdle;
struct IIdleListener;

struct BackendTimer;
struct ITimerListener;

struct BackendAsync;
struct IAsyncListener;

struct BackendSignal;
struct ISignalListener;

struct BackendPoll;
struct IPollListener;

struct BackendUdp;
struct IUdpListener;
struct ISendListener;
struct BackendSendRequest;

struct IStreamListener;
struct BackendPipe;

}}};
