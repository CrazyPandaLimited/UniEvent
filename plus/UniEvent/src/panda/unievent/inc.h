#pragma once
#include <type_traits>

#define PEXS_NULL_TERMINATE(what, to)            \
    char to[what.length()+1];                    \
    std::memcpy(to, what.data(), what.length()); \
    to[what.length()] = 0;

namespace panda { namespace unievent {

//typedef uv_os_sock_t sock_t;
//typedef uv_file      file_t;
//typedef uv_uid_t     uid_t;
//typedef uv_gid_t     gid_t;
//typedef uv_stat_t    stat_t;
//
//typedef typename std::underlying_type<errno_t>::type errno_underlying_t;

}}
