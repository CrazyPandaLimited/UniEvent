#pragma once
#include <iosfwd>
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
    using fd_t   = int;
    using sock_t = SOCKET;
    using fh_t   = HANDLE;
#else
    using fd_t   = int;
    using sock_t = int;
    using fh_t   = int;
#endif

enum class Ownership {
    TRANSFER = 0,
    SHARE
};

struct TimeVal {
  int64_t sec;
  int32_t usec;

  double get () const { return (double)sec + (double)usec / 1000000; }

  bool operator== (const TimeVal& oth) const { return sec == oth.sec && usec == oth.usec; }
  bool operator!= (const TimeVal& oth) const { return !operator==(oth); }
  bool operator>= (const TimeVal& oth) const { return sec > oth.sec || (sec == oth.sec && usec >= oth.usec); }
  bool operator>  (const TimeVal& oth) const { return sec > oth.sec || (sec == oth.sec && usec > oth.usec); }
  bool operator<= (const TimeVal& oth) const { return !operator>(oth); }
  bool operator<  (const TimeVal& oth) const { return !operator>=(oth); }
};
std::ostream& operator<< (std::ostream& os, const TimeVal&);

struct TimeSpec {
    long sec;
    long nsec;

    double get () const { return (double)sec + (double)nsec / 1000000000; }

    bool operator== (const TimeSpec& oth) const { return sec == oth.sec && nsec == oth.nsec; }
    bool operator!= (const TimeSpec& oth) const { return !operator==(oth); }
    bool operator>= (const TimeSpec& oth) const { return sec > oth.sec || (sec == oth.sec && nsec >= oth.nsec); }
    bool operator>  (const TimeSpec& oth) const { return sec > oth.sec || (sec == oth.sec && nsec > oth.nsec); }
    bool operator<= (const TimeSpec& oth) const { return !operator>(oth); }
    bool operator<  (const TimeSpec& oth) const { return !operator>=(oth); }
};
std::ostream& operator<< (std::ostream& os, const TimeSpec&);

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
