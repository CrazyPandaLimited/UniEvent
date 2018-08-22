#pragma once
#include <uv.h>
#include <type_traits>

namespace panda { namespace unievent {

typedef uv_os_sock_t sock_t;
typedef uv_file      file_t;
typedef uv_uid_t     uid_t;
typedef uv_gid_t     gid_t;
typedef uv_stat_t    stat_t;

typedef enum {
#define PEV__ERRNO_GEN(name,val) ERRNO_##name = UV_##name,
  UV_ERRNO_MAP(PEV__ERRNO_GEN)
#undef PEV__ERRNO_GEN
  ERRNO_SSL = UV_ERRNO_MAX,
  ERRNO_SOCKS = UV_ERRNO_MAX+1,
  ERRNO_MAX = UV_ERRNO_MAX+2
} errno_t;

typedef typename std::underlying_type<errno_t>::type errno_underlying_t;

}}
