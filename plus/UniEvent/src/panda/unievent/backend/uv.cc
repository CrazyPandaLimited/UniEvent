#include "uv.h"
#include "uv/UVBackend.h"

namespace panda { namespace unievent { namespace backend {

static Backend _backend;

Backend* UV = &_backend;

}}}
