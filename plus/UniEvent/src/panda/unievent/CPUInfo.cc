#include <panda/unievent/CPUInfo.h>
using namespace panda::unievent;

#define _CI_FREE_ if (cnt) { uv_free_cpu_info(cpu_infos, cnt); cnt = 0; cpu_infos = nullptr; }

void CPUInfo::refresh () {
    _CI_FREE_;
    int err = uv_cpu_info(&cpu_infos, &cnt);
    if (err) throw OperationError(err);
}

CPUInfo::~CPUInfo () {
    _CI_FREE_;
}
