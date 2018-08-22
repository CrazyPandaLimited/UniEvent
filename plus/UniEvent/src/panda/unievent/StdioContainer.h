#pragma once
#include <panda/unievent/inc.h>
#include <panda/unievent/Stream.h>

namespace panda { namespace unievent {

class StdioContainer {
public:
    enum stdio_flags {
      FL_IGNORE         = UV_IGNORE,
      FL_CREATE_PIPE    = UV_CREATE_PIPE,
      FL_INHERIT_FD     = UV_INHERIT_FD,
      FL_INHERIT_STREAM = UV_INHERIT_STREAM,
      FL_READABLE_PIPE  = UV_READABLE_PIPE,
      FL_WRITABLE_PIPE  = UV_WRITABLE_PIPE
    };

    StdioContainer (int fd, stdio_flags flags) {
        cont.data.fd = fd;
        cont.flags = static_cast<uv_stdio_flags>(flags);
    }

    StdioContainer (Stream* stream, stdio_flags flags) {
        cont.data.stream = _pex_(stream);
        cont.flags = static_cast<uv_stdio_flags>(flags);
    }

private:
    uv_stdio_container_t cont;
};

}}
