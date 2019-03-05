#pragma once
#include "../forward.h"

namespace panda { namespace unievent { namespace backend {

struct BackendLoop;
struct BackendHandle;

struct ITimerListener;
struct BackendTimer;

struct IPrepareListener;
struct BackendPrepare;

struct ICheckListener;
struct BackendCheck;

struct IIdleListener;
struct BackendIdle;

struct IAsyncListener;
struct BackendAsync;

struct ISignalListener;
struct BackendSignal;

struct IPollListener;
struct BackendPoll;

struct ITickListener;
struct BackendTick;

}}};
