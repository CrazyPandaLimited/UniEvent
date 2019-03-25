#pragma once
#include <cstdint>

#if defined(_WIN32)
    #include <winsock2.h>
    namespace panda { namespace unievent {
        using file_t = int;
        using sock_t = SOCKET;
        using fd_t   = HANDLE;
    }}
#else
    namespace panda { namespace unievent {
        using file_t = int;
        using sock_t = int;
        using fd_t   = int;
    }}
#endif
