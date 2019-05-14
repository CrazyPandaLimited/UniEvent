#pragma once
#include <stdint.h>
#include <type_traits>
#if defined(_WIN32)
    #include <winsock2.h>
#endif

#define UE_NULL_TERMINATE(what, to)              \
    char to[what.length()+1];                    \
    std::memcpy(to, what.data(), what.length()); \
    to[what.length()] = 0;

namespace panda { namespace unievent {

#if defined(_WIN32)
    using file_t = int;
    using sock_t = SOCKET;
    using fd_t   = HANDLE;
#else
    using file_t = int;
    using sock_t = int;
    using fd_t   = int;
#endif

enum class Ownership {
    TRANSFER = 0,
    SHARE
};

struct TimeVal {
  long sec;
  long usec;
};

struct Stat {
  uint64_t dev;
  uint64_t mode;
  uint64_t nlink;
  uint64_t uid;
  uint64_t gid;
  uint64_t rdev;
  uint64_t ino;
  uint64_t size;
  uint64_t blksize;
  uint64_t blocks;
  uint64_t flags;
  uint64_t gen;
  TimeVal  atime;
  TimeVal  mtime;
  TimeVal  ctime;
  TimeVal  birthtime;
};

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
