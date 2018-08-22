#pragma once
#include <panda/unievent/Error.h>

namespace panda { namespace unievent {

typedef uv_cpu_info_t cpu_info_t;

class CPUInfo {
public:
    CPUInfo () : cnt(0), cpu_infos(nullptr) { refresh(); }

    virtual void refresh ();

    const cpu_info_t* list  () const { return cpu_infos; }
    int               count () const { return cnt; }

    virtual ~CPUInfo ();

private:
    int         cnt;
    cpu_info_t* cpu_infos;
};

}}
