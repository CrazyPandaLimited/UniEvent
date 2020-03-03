#pragma once
#include "inc.h"
#include <system_error>
#include <panda/error.h>
#include <panda/string.h>
#include <panda/exception.h>
#include <panda/string_view.h>

namespace panda { namespace unievent {

enum class errc {
    ssl_error = 1,
    resolve_error,
    ai_address_family_not_supported, //
    ai_temporary_failure, //
    ai_bad_flags, //
    ai_bad_hints, //
    ai_request_canceled, //
    ai_permanent_failure, //
    ai_family_not_supported, //
    ai_out_of_memory, //
    ai_no_address, //
    ai_unknown_node_or_service, //
    ai_argument_buffer_overflow, //
    ai_resolved_protocol_unknown, //
    ai_service_not_available_for_socket_type, //
    ai_socket_type_not_supported, //
    invalid_unicode_character, //
    not_on_network, //
    transport_endpoint_shutdown, //
    unknown_error, //
    host_down, //
    remote_io, //
};

struct ErrorCategory : std::error_category {
    const char* name () const throw() override;
    std::string message (int condition) const throw() override;
};
struct SslErrorCategory : std::error_category {
    const char* name () const throw() override;
    std::string message (int condition) const throw() override;
};
struct OpenSslErrorCategory : std::error_category {
    const char* name () const throw() override;
    std::string message (int condition) const throw() override;
};

extern const ErrorCategory        error_category;
extern const SslErrorCategory     ssl_error_category;
extern const OpenSslErrorCategory openssl_error_category;

inline std::error_code make_error_code (errc code) { return std::error_code((int)code, error_category); }

std::error_code make_ssl_error_code (int ssl_code);


struct Error : panda::exception {
    using exception::exception;
    Error (const ErrorCode& ec);

    const ErrorCode& code () const;

    virtual string whats () const noexcept override;
    virtual Error* clone () const;

protected:
    ErrorCode ec;
};

}}

namespace std {
    template <> struct is_error_code_enum<panda::unievent::errc> : true_type {};
}
