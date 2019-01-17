#pragma once
#include "inc.h"
#include "Error.h"
#include <vector>

namespace panda { namespace unievent {

struct CpuInfo {
    string model;
    int    speed;
    struct CpuTimes {
        uint64_t user;
        uint64_t nice;
        uint64_t sys;
        uint64_t idle;
        uint64_t irq;
    } cpu_times;
};

//struct TimeVal {
//  long sec;
//  long usec;
//};
//
//struct ResourceUsage {
//   TimeVal  utime;    /* user CPU time used */
//   TimeVal  stime;    /* system CPU time used */
//   uint64_t maxrss;   /* maximum resident set size */
//   uint64_t ixrss;    /* integral shared memory size */
//   uint64_t idrss;    /* integral unshared data size */
//   uint64_t isrss;    /* integral unshared stack size */
//   uint64_t minflt;   /* page reclaims (soft page faults) */
//   uint64_t majflt;   /* page faults (hard page faults) */
//   uint64_t nswap;    /* swaps */
//   uint64_t inblock;  /* block input operations */
//   uint64_t oublock;  /* block output operations */
//   uint64_t msgsnd;   /* IPC messages sent */
//   uint64_t msgrcv;   /* IPC messages received */
//   uint64_t nsignals; /* signals received */
//   uint64_t nvcsw;    /* voluntary context switches */
//   uint64_t nivcsw;   /* involuntary context switches */
//};


std::vector<CpuInfo> cpu_info ();

//panda::string hostname            ();
//size_t        resident_set_memory ();
//uint64_t      get_free_memory     ();
//uint64_t      get_total_memory    ();
//ResourceUsage get_rusage          ();

CodeError uvx_code_error (int uverr);

//inline bool inet_looks_like_ipv6 (const char* src) {
//    while (*src) if (*src++ == ':') return true;
//    return false;
//}

}}

