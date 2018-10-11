#include "SockAddr.h"
#include "global.h"

namespace panda { namespace unievent {

static const int MAX_IP4_ADDRSTRLEN = 16;
static const int MAX_IP6_ADDRSTRLEN = 46;

SockAddr::SockAddr (const SockAddr& oth) {
    switch (oth.sa.sa_family) {
        case AF_INET  : sa4 = oth.sa4; break;
        case AF_INET6 : sa6 = oth.sa6; break;
        case AF_UNIX  : sau = oth.sau; break;
        default       : throw "should not happen";
    }
}

SockAddr::Inet4::Inet4 (const std::string_view& ip, uint16_t port) {
    PEXS_NULL_TERMINATE(ip, ipstr);
    auto err = uv_ip4_addr(ipstr, port, &sa4);
    if (err) throw CodeError(err);
}

SockAddr::Inet4::Inet4 (const in_addr& addr, uint16_t port) {
    memset(&sa4, 0, sizeof(sa4));
    sa4.sin_family = AF_INET;
    sa4.sin_port = htons(port);
    sa4.sin_addr = addr;
}

string SockAddr::Inet4::ip () const {
    string ret(MAX_IP4_ADDRSTRLEN);
    auto err = uv_inet_ntop(AF_INET, &sa4.sin_addr, ret.buf(), MAX_IP4_ADDRSTRLEN);
    assert(!err);
    return ret;
}

SockAddr::Inet6::Inet6 (const std::string_view& ip, uint16_t port) {
    PEXS_NULL_TERMINATE(ip, ipstr);
    auto err = uv_ip6_addr(ipstr, port, &sa6);
    if (err) throw CodeError(err);
}

SockAddr::Inet6::Inet6 (const in6_addr& addr, uint16_t port) {
    memset(&sa6, 0, sizeof(sa6));
    sa6.sin6_family = AF_INET6;
    sa6.sin6_port = htons(port);
    sa6.sin6_addr = addr;
}

string SockAddr::Inet6::ip () const {
    string ret(MAX_IP6_ADDRSTRLEN);
    auto err = uv_inet_ntop(AF_INET6, &sa6.sin6_addr, ret.buf(), MAX_IP6_ADDRSTRLEN);
    assert(!err);
    return ret;
}

#ifndef _WIN32

SockAddr::Unix::Unix (const std::string_view& path) {
    if (path.length() >= sizeof(sau.sun_path)) throw CodeError(ERRNO_EINVAL);
    sau.sun_family = AF_UNIX;
    memcpy(sau.sun_path, path.data(), path.length());
    sau.sun_path[path.length()] = 0;
}

#endif

}}
