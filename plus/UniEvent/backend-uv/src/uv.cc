#include <panda/unievent/backend/uv.h>
#include "UVBackend.h"
#include "UVUdp.h"
#include "UVTcp.h"
#include "UVTty.h"
#include "UVIdle.h"
#include "UVPoll.h"
#include "UVPipe.h"
#include "UVTimer.h"
#include "UVCheck.h"
#include "UVAsync.h"
#include "UVSignal.h"
#include "UVDelayer.h"
#include "UVPrepare.h"

namespace panda { namespace unievent { namespace backend {

static uv::UVBackend _backend;

Backend* UV = &_backend;

}}}

namespace panda { namespace unievent { namespace backend { namespace uv {

BackendTimer*   UVLoop::new_timer     (ITimerListener* l)              { return new UVTimer(this, l); }
BackendPrepare* UVLoop::new_prepare   (IPrepareListener* l)            { return new UVPrepare(this, l); }
BackendCheck*   UVLoop::new_check     (ICheckListener* l)              { return new UVCheck(this, l); }
BackendIdle*    UVLoop::new_idle      (IIdleListener* l)               { return new UVIdle(this, l); }
BackendAsync*   UVLoop::new_async     (IAsyncListener* l)              { return new UVAsync(this, l); }
BackendSignal*  UVLoop::new_signal    (ISignalListener* l)             { return new UVSignal(this, l); }
BackendPoll*    UVLoop::new_poll_sock (IPollListener* l, sock_t sock)  { return new UVPoll(this, l, sock); }
BackendPoll*    UVLoop::new_poll_fd   (IPollListener* l, int fd)       { return new UVPoll(this, l, fd, nullptr); }
BackendUdp*     UVLoop::new_udp       (IUdpListener* l, int domain)    { return new UVUdp(this, l, domain); }
BackendPipe*    UVLoop::new_pipe      (IStreamListener* l, bool ipc)   { return new UVPipe(this, l, ipc); }
BackendTcp*     UVLoop::new_tcp       (IStreamListener* l, int domain) { return new UVTcp(this, l, domain); }
BackendTty*     UVLoop::new_tty       (IStreamListener* l, file_t fd)  { return new UVTty(this, l, fd); }

}}}}
