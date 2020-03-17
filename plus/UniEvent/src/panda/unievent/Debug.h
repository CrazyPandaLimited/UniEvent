#pragma once
#include <panda/log.h>

namespace panda { namespace unievent {
    extern panda::log::Module uelog;
}}

#define _ECTOR() do { panda_mlog_verbose_debug(panda::unievent::uelog, __func__ << " [ctor]" ); } while(0)
#define _EDTOR() do { panda_mlog_verbose_debug(panda::unievent::uelog, __func__ << " [dtor]" ); } while(0)
