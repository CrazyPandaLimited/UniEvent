#pragma once
#include "inc.h"
#include <stdexcept>
#include <system_error>
#include <panda/lib.h>
#include <panda/string.h>
#include <panda/string_view.h>

namespace panda { namespace unievent {

enum class errc {
    ssl_error = 1,
    socks_error,
    resolve_error,
    ai_address_family_not_supported,
    ai_temporary_failure,
    ai_bad_flags,
    ai_bad_hints,
    ai_request_canceled,
    ai_permanent_failure,
    ai_family_not_supported,
    ai_out_of_memory,
    ai_no_address,
    ai_unknown_node_or_service,
    ai_argument_buffer_overflow,
    ai_resolved_protocol_unknown,
    ai_service_not_available_for_socket_type,
    ai_socket_type_not_supported,
    invalid_unicode_character,
    not_on_network,
    transport_endpoint_shutdown,
    unknown_error,
    host_down,
    remote_io
};

struct ErrorCategory : std::error_category {
    const char* name () const throw() override;
    std::string message (int condition) const throw() override;
};

struct Error : std::exception {
    Error () {}
    Error (const string&);
    virtual const char*   what  () const throw() override;
    virtual const string& whats () const;
    virtual Error*        clone () const;
protected:
    mutable string _what;
    virtual string _mkwhat () const;
};

struct CodeError : Error {
    CodeError () {}
    CodeError (errc code);
    CodeError (std::errc code);
    CodeError (const std::error_code& code);

    const std::error_code& code () const;

    virtual string descr () const;

    virtual CodeError* clone () const override;

    explicit
    operator bool () const { return _code.value(); }

    static ErrorCategory category;

    bool operator== (const CodeError& oth) const { return code() == oth.code(); }
    bool operator!= (const CodeError& oth) const { return !operator==(oth); }

protected:
    std::error_code _code;

    string _mkwhat () const override;
};

struct SSLError : CodeError {
    SSLError (int ssl_code);
    SSLError (int ssl_code, unsigned long openssl_code);

    int ssl_code     () const;
    int openssl_code () const;

    int library  () const;
    int function () const;
    int reason   () const;

    string library_str  () const;
    string function_str () const;
    string reason_str   () const;

    string descr () const override;

    virtual SSLError* clone () const override;

private:
    int           _ssl_code;
    unsigned long _openssl_code;
};

}}
