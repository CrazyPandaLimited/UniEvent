#pragma once

#include <panda/cast.h>
#include <panda/unievent/inc.h>
#include <panda/unievent/Error.h>


namespace panda { namespace unievent {

typedef uv_rusage_t rusage_t;

inline char** setup_args (int argc, char** argv) { return uv_setup_args(argc, argv); }

inline size_t resident_set_memory () {
    size_t rss;
    int err = uv_resident_set_memory(&rss);
    if (err) throw CodeError(err);
    return rss;
}

inline double uptime () {
    double uptime;
    int err = uv_uptime(&uptime);
    if (err) throw CodeError(err);
    return uptime;
}

inline bool inet_looks_like_ipv6 (const char* src) {
    while (*src) if (*src++ == ':') return true;
    return false;
}

inline void inet_pton (const char* src, in_addr* dst) {
    int err = uv_inet_pton(AF_INET, src, dst);
    if (err) throw CodeError(err);
}

inline void inet_pton (const char* src, in6_addr* dst) {
    int err = uv_inet_pton(AF_INET6, src, dst);
    if (err) throw CodeError(err);
}

inline void inet_ntop (in_addr* src, char* dst, size_t size) {
    int err = uv_inet_ntop(AF_INET, src, dst, size);
    if (err) throw CodeError(err);
}

inline void inet_ntop (in6_addr* src, char* dst, size_t size) {
    int err = uv_inet_ntop(AF_INET6, src, dst, size);
    if (err) throw CodeError(err);
}

inline void inet_ptos (const char* ip, int port, sockaddr_in* sa)  {
    int err = uv_ip4_addr(ip, port, sa);
    if (err) throw CodeError(err);
}

inline void inet_ptos (const char* ip, int port, sockaddr_in6* sa) {
    int err = uv_ip6_addr(ip, port, sa);
    if (err) throw CodeError(err);
}

inline void inet_stop (struct sockaddr_in* src, char* dst, size_t size, uint16_t* port = nullptr)  {
    int err = uv_ip4_name(src, dst, size);
    if (err) throw CodeError(err);
    if (port) *port = ntohs(src->sin_port);
}

inline void inet_stop (struct sockaddr_in6* src, char* dst, size_t size, uint16_t* port = nullptr)  {
    int err = uv_ip6_name(src, dst, size);
    if (err) throw CodeError(err);
    if (port) *port = ntohs(src->sin6_port);
}

inline void inet_stop (struct sockaddr* src, char* dst, size_t size) {
    src->sa_family == PF_INET6 ? inet_stop((struct sockaddr_in6*)src, dst, size) : inet_stop((struct sockaddr_in*)src, dst, size);
}

inline void exepath (char* buffer, size_t* size) throw(CodeError) {
    if (uv_exepath(buffer, size)) throw CodeError(UV_UNKNOWN);
}

inline void cwd (char* buffer, size_t* size) {
    int err = uv_cwd(buffer, size);
    if (err) throw CodeError(err);
}

inline void chdir (const char* dir) {
    int err = uv_chdir(dir);
    if (err) throw CodeError(err);
}

inline void os_homedir (char* buffer, size_t* size) {
    int err = uv_os_homedir(buffer, size);
    if (err) throw CodeError(err);
}

inline void os_tmpdir (char* buffer, size_t* size) {
    int err = uv_os_tmpdir(buffer, size);
    if (err) throw CodeError(err);
}

panda::string hostname ();

inline uint64_t get_free_memory  () { return uv_get_free_memory(); }
inline uint64_t get_total_memory () { return uv_get_total_memory(); }

inline uint64_t hrtime() { return uv_hrtime(); }

inline void get_rusage (rusage_t* rusage) {
    int err = uv_getrusage(rusage);
    if (err) throw CodeError(err);
}

inline void loadavg (double avg[3]) { uv_loadavg(avg); }

inline void disable_stdio_inheritance () { return uv_disable_stdio_inheritance(); }

extern "C" {
    int uv__getaddrinfo_translate_error(int sys_err);
}
inline uv_errno_t _err_gai2uv (int syscode) {
    return (uv_errno_t) uv__getaddrinfo_translate_error(syscode);
}

class Handle; class Request;
template <class T, class X> static T hcast (X* h) { return panda::dyn_cast<T>(static_cast<Handle*>(h->data)); }
template <class T, class X> static T rcast (X* r) { return panda::dyn_cast<T>(static_cast<Request*>(r->data)); }

#define PEXS_NULL_TERMINATE(what, to)            \
    char to[what.length()+1];                      \
    std::memcpy(to, what.data(), what.length()); \
    to[what.length()] = 0;

}}

