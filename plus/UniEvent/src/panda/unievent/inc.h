#pragma once
#include <type_traits>
#if defined(_WIN32)
#   include "inc-win.h"
#else
#   include "inc-unix.h"
#endif

#define PEXS_NULL_TERMINATE(what, to)            \
    char to[what.length()+1];                    \
    std::memcpy(to, what.data(), what.length()); \
    to[what.length()] = 0;
