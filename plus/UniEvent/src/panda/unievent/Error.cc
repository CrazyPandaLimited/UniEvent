#include "Error.h"
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace panda { namespace unievent {

const char* ErrorCategory::name () const throw() { return "unievent"; }

std::string ErrorCategory::message (int condition) const throw() {
    switch ((errc)condition) {
        case errc::ssl_error                                : return "ssl error";
        case errc::socks_error                              : return "socks error";
        case errc::resolve_error                            : return "resolve error";
        case errc::ai_address_family_not_supported          : return "address family not supported";
        case errc::ai_temporary_failure                     : return "temporary failure";
        case errc::ai_bad_flags                             : return "bad ai_flags value";
        case errc::ai_bad_hints                             : return "invalid value for hints";
        case errc::ai_request_canceled                      : return "request canceled";
        case errc::ai_permanent_failure                     : return "permanent failure";
        case errc::ai_family_not_supported                  : return "ai_family not supported";
        case errc::ai_out_of_memory                         : return "out of memory";
        case errc::ai_no_address                            : return "no address";
        case errc::ai_unknown_node_or_service               : return "unknown node or service";
        case errc::ai_argument_buffer_overflow              : return "argument buffer overflow";
        case errc::ai_resolved_protocol_unknown             : return "resolved protocol is unknown";
        case errc::ai_service_not_available_for_socket_type : return "service not available for socket type";
        case errc::ai_socket_type_not_supported             : return "socket type not supported";
        case errc::invalid_unicode_character                : return "invalid Unicode character";
        case errc::not_on_network                           : return "machine is not on the network";
        case errc::transport_endpoint_shutdown              : return "cannot send after transport endpoint shutdown";
        case errc::unknown_error                            : return "unknown error";
        case errc::host_down                                : return "host is down";
        case errc::remote_io                                : return "remote I/O error";
    }
    return {};
}

Error::Error (const string& what) : _what(what) {}

string        Error::_mkwhat () const         { return string(); }
const char*   Error::what    () const throw() { return whats().c_str(); }
const string& Error::whats   () const         { if (!_what) _what = _mkwhat(); return _what; }
Error*        Error::clone   () const         { return new Error(_what); }


ErrorCategory CodeError::category;

CodeError::CodeError (errc      value) : _code(std::error_code((int)value, category)) {}
CodeError::CodeError (std::errc value) : _code(make_error_code(value))                {}

CodeError::CodeError (const std::error_code& code) : _code(code) {}

const std::error_code& CodeError::code () const { return _code; }

string CodeError::descr () const {
    auto stds = _code.message();
    return string(stds.data(), stds.length());
}

string CodeError::_mkwhat () const {
    if (!_code) return {};
    return string(_code.category().name()) + ':' + string::from_number(_code.value()) + " " + descr();
}

CodeError* CodeError::clone () const { return new CodeError(_code); }


SSLError::SSLError (int ssl_code) : SSLError(ssl_code, 0) {
    if (_ssl_code == SSL_ERROR_SSL) {
        unsigned long tmp;
        while ((tmp = ERR_get_error())) _openssl_code = tmp;
    }
}

SSLError::SSLError (int ssl_code, unsigned long openssl_code) : CodeError(errc::ssl_error), _ssl_code(ssl_code), _openssl_code(openssl_code) {}

int SSLError::ssl_code     () const { return _ssl_code; }
int SSLError::openssl_code () const { return _openssl_code; }

int SSLError::library  () const { return _ssl_code == SSL_ERROR_SSL ? ERR_GET_LIB(_openssl_code) : 0; }
int SSLError::function () const { return _ssl_code == SSL_ERROR_SSL ? ERR_GET_FUNC(_openssl_code) : 0; }
int SSLError::reason   () const { return _ssl_code == SSL_ERROR_SSL ? ERR_GET_REASON(_openssl_code) : 0; }

string SSLError::library_str () const {
    if (_ssl_code != SSL_ERROR_SSL) return {};
    const char* str = ERR_lib_error_string(_openssl_code);
    return str ? string(str) : string();
}

string SSLError::function_str () const {
    if (_ssl_code != SSL_ERROR_SSL) return {};
    const char* str = ERR_func_error_string(_openssl_code);
    return str ? string(str) : string();
}

string SSLError::reason_str () const {
    if (_ssl_code != SSL_ERROR_SSL) return {};
    const char* str = ERR_reason_error_string(_openssl_code);
    return str ? string(str) : string();
}

string SSLError::descr () const {
    if (_ssl_code != SSL_ERROR_SSL) return {};
    string ret(120);
    char* buf = ret.buf();
    ERR_error_string_n(_openssl_code, buf, ret.capacity());
    ret.length(strlen(buf));
    return ret;
}

SSLError* SSLError::clone () const { return new SSLError(_ssl_code, _openssl_code); }

}}
