#pragma once
#include "inc.h"
#include <panda/string.h>
#include <panda/string_view.h>
#ifndef _WIN32
  #include <sys/un.h>
#endif

namespace panda { namespace unievent {

struct SockAddr {
    struct Inet4;
    struct Inet6;

    SockAddr (const sockaddr_in&  sa) : sa4(sa) {}
    SockAddr (const sockaddr_in6& sa) : sa6(sa) {}

    SockAddr (const SockAddr&);

    sa_family_t family () const { return sa.sa_family; }

    bool is_inet4 () const { return family() == AF_INET; }
    bool is_inet6 () const { return family() == AF_INET6; }

    Inet4& inet4 () { return *((Inet4*)this); }
    Inet6& inet6 () { return *((Inet6*)this); }

    #ifndef _WIN32

    struct Unix;

    SockAddr (const sockaddr_un&  sa) : sau(sa) {}

    bool is_unix () const { return family() == AF_UNIX; }

    Unix& unix () { return *((Unix*)this); }

    #endif

protected:
    SockAddr () {}

    union {
        sockaddr     sa;
        sockaddr_in  sa4;
        sockaddr_in6 sa6;
        #ifndef _WIN32
        sockaddr_un  sau;
        #endif
    };
};

struct SockAddr::Inet4 : SockAddr {
    Inet4 () {
        memset(&sa4, 0, sizeof(sa4));
        sa4.sin_family = AF_INET;
    }

    Inet4 (const sockaddr_in& sa) : SockAddr(sa) {}

    Inet4 (const std::string_view& ip, uint16_t port);
    Inet4 (const in_addr& addr, uint16_t port);

    const in_addr& addr () const { return sa4.sin_addr; }

    uint16_t port () const { return ntohs(sa4.sin_port); }
    string   ip   () const;
};

struct SockAddr::Inet6 : SockAddr {
    Inet6 () {
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
    }

    Inet6 (const sockaddr_in6& sa) : SockAddr(sa) {}

    Inet6 (const std::string_view& ip, uint16_t port);
    Inet6 (const in6_addr& addr, uint16_t port);

    const in6_addr& addr () const { return sa6.sin6_addr; }

    uint16_t port () const { return ntohs(sa6.sin6_port); }
    string   ip   () const;
};

#ifndef _WIN32

struct SockAddr::Unix : SockAddr {
    Unix () {
        sau.sun_family = AF_UNIX;
        sau.sun_path[0] = 0;
    }

    Unix (const sockaddr_un& sa) : SockAddr(sa) {}

    Unix (const std::string_view& path);

    std::string_view path () const { return (char*)sau.sun_path; }
};

#endif

}}
