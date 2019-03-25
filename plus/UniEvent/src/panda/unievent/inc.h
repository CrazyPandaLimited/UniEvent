#pragma once
#include <type_traits>

#define PEXS_NULL_TERMINATE(what, to)            \
    char to[what.length()+1];                    \
    std::memcpy(to, what.data(), what.length()); \
    to[what.length()] = 0;

namespace panda { namespace unievent {

template <class F1, class F2>
void scope_guard (F1&& code, F2&& guard) {
    try { code(); }
    catch (...) {
        guard();
        throw;
    }
    guard();
}

}}
