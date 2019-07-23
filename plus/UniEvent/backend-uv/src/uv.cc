#include <panda/unievent/backend/uv.h>
#include "UVBackend.h"
#include "UVUdp.h"
#include "UVTcp.h"
#include "UVTty.h"
#include "UVIdle.h"
#include "UVPoll.h"
#include "UVPipe.h"
#include "UVWork.h"
#include "UVTimer.h"
#include "UVCheck.h"
#include "UVAsync.h"
#include "UVSignal.h"
#include "UVDelayer.h"
#include "UVPrepare.h"
#include "UVFsEvent.h"

namespace panda { namespace unievent { namespace backend {

static uv::UVBackend _backend;

Backend* UV = &_backend;

}}}

namespace panda { namespace unievent { namespace backend { namespace uv {

TimerImpl*   UVLoop::new_timer     (ITimerListener* l)              { return new UVTimer(this, l); }
PrepareImpl* UVLoop::new_prepare   (IPrepareListener* l)            { return new UVPrepare(this, l); }
CheckImpl*   UVLoop::new_check     (ICheckListener* l)              { return new UVCheck(this, l); }
IdleImpl*    UVLoop::new_idle      (IIdleListener* l)               { return new UVIdle(this, l); }
AsyncImpl*   UVLoop::new_async     (IAsyncListener* l)              { return new UVAsync(this, l); }
SignalImpl*  UVLoop::new_signal    (ISignalListener* l)             { return new UVSignal(this, l); }
PollImpl*    UVLoop::new_poll_sock (IPollListener* l, sock_t sock)  { return new UVPoll(this, l, sock); }
PollImpl*    UVLoop::new_poll_fd   (IPollListener* l, int fd)       { return new UVPoll(this, l, fd, nullptr); }
UdpImpl*     UVLoop::new_udp       (IUdpListener* l, int domain)    { return new UVUdp(this, l, domain); }
PipeImpl*    UVLoop::new_pipe      (IStreamListener* l, bool ipc)   { return new UVPipe(this, l, ipc); }
TcpImpl*     UVLoop::new_tcp       (IStreamListener* l, int domain) { return new UVTcp(this, l, domain); }
TtyImpl*     UVLoop::new_tty       (IStreamListener* l, fd_t fd)    { return new UVTty(this, l, fd); }
WorkImpl*    UVLoop::new_work      (IWorkListener* l)               { return new UVWork(this, l); }
FsEventImpl* UVLoop::new_fs_event  (IFsEventListener* l)            { return new UVFsEvent(this, l); }

}}}}
