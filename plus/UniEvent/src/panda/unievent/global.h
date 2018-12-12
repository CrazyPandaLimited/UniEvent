#pragma once
#include "inc.h"
#include "Fwd.h"
#include "Error.h"
#include <panda/cast.h>

namespace panda { namespace unievent {

//typedef uv_rusage_t rusage_t;
//
//inline char** setup_args (int argc, char** argv) { return uv_setup_args(argc, argv); }
//
//inline size_t resident_set_memory () {
//    size_t rss;
//    int err = uv_resident_set_memory(&rss);
//    if (err) throw CodeError(err);
//    return rss;
//}
//
//inline double uptime () {
//    double uptime;
//    int err = uv_uptime(&uptime);
//    if (err) throw CodeError(err);
//    return uptime;
//}
//
//inline bool inet_looks_like_ipv6 (const char* src) {
//    while (*src) if (*src++ == ':') return true;
//    return false;
//}
//
//inline void exepath (char* buffer, size_t* size) {
//    if (uv_exepath(buffer, size)) throw CodeError(UV_UNKNOWN);
//}
//
//inline void cwd (char* buffer, size_t* size) {
//    int err = uv_cwd(buffer, size);
//    if (err) throw CodeError(err);
//}
//
//inline void chdir (const char* dir) {
//    int err = uv_chdir(dir);
//    if (err) throw CodeError(err);
//}
//
//inline void os_homedir (char* buffer, size_t* size) {
//    int err = uv_os_homedir(buffer, size);
//    if (err) throw CodeError(err);
//}
//
//inline void os_tmpdir (char* buffer, size_t* size) {
//    int err = uv_os_tmpdir(buffer, size);
//    if (err) throw CodeError(err);
//}
//
//panda::string hostname ();
//
//inline uint64_t get_free_memory  () { return uv_get_free_memory(); }
//inline uint64_t get_total_memory () { return uv_get_total_memory(); }
//
//inline uint64_t hrtime() { return uv_hrtime(); }
//
//inline void get_rusage (rusage_t* rusage) {
//    int err = uv_getrusage(rusage);
//    if (err) throw CodeError(err);
//}
//
//inline void loadavg (double avg[3]) { uv_loadavg(avg); }
//
//inline void disable_stdio_inheritance () { return uv_disable_stdio_inheritance(); }
//
//extern "C" {
//    int uv__getaddrinfo_translate_error(int sys_err);
//}
//inline uv_errno_t _err_gai2uv (int syscode) {
//    return (uv_errno_t) uv__getaddrinfo_translate_error(syscode);
//}
//
//template <class T, class X> static T hcast (X* h) { return panda::dyn_cast<T>(static_cast<Handle*>(h->data)); }
//template <class T, class X> static T rcast (X* r) { return panda::dyn_cast<T>(static_cast<Request*>(r->data)); }

#define PEXS_NULL_TERMINATE(what, to)            \
    char to[what.length()+1];                      \
    std::memcpy(to, what.data(), what.length()); \
    to[what.length()] = 0;

}}

