#pragma once

namespace panda { namespace unievent { namespace backend {

struct LoopImpl;
struct HandleImpl;

struct PrepareImpl;
struct IPrepareListener;

struct CheckImpl;
struct ICheckListener;

struct IdleImpl;
struct IIdleListener;

struct TimerImpl;
struct ITimerListener;

struct AsyncImpl;
struct IAsyncListener;

struct SignalImpl;
struct ISignalListener;

struct PollImpl;
struct IPollListener;

struct UdpImpl;
struct IUdpListener;
struct ISendListener;
struct SendRequestImpl;

struct StreamImpl;
struct IStreamListener;
struct IConnectListener;
struct ConnectRequestImpl;
struct IWriteListener;
struct WriteRequestImpl;
struct IShutdownListener;
struct ShutdownRequestImpl;

struct PipeImpl;
struct TcpImpl;
struct TtyImpl;

struct WorkImpl;
struct IWorkListener;

struct FsEventImpl;
struct IFsEventListener;

}}};
