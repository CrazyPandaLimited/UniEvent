#pragma once

#include <panda/excepted.h>
#include <panda/error.h>
#include <panda/net/sockaddr.h>

namespace panda {
namespace unievent {

enum class NotConnectedError {
    Ignore,
    Process
};

inline excepted<net::SockAddr, ErrorCode> handle_sockaddr(const excepted<net::SockAddr, std::error_code>& r, NotConnectedError strategy) {
    if (strategy == NotConnectedError::Process) {
        return r.map_error([](const auto& e) { return ErrorCode(e); });
    }
    if (r.has_value()) {
        return r.value();
    }
    std::error_code err = r.error();
    if (err == std::errc::not_connected || err == std::errc::bad_file_descriptor || err == std::errc::invalid_argument) {
        return net::SockAddr{};
    };
    return make_unexpected(err);
}

}

}
