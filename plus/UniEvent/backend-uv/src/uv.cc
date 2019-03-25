#include <panda/unievent/backend/uv.h>
#include "UVBackend.h"
#include <panda/unievent/Udp.h>

namespace panda { namespace unievent { namespace backend {

static uv::UVBackend _backend;

Backend* UV = &_backend;

}}}
