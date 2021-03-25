#pragma once
#include <panda/excepted.h>
#include <panda/error.h>
#include <panda/net/sockaddr.h>

namespace panda { namespace unievent {

enum class NotConnectedStrategy {
    Ignore,
    Error
};

template <class T>
inline excepted<T, ErrorCode> handle_sockexc (const excepted<T, std::error_code>& r, NotConnectedStrategy strategy) {
    if (r.has_value()) return r.value();

    std::error_code err = r.error();
    if (strategy == NotConnectedStrategy::Ignore &&
        (err == std::errc::not_connected || err == std::errc::bad_file_descriptor || err == std::errc::invalid_argument)
    ) {
        return {};
    }

    return make_unexpected(err);
}

}}
