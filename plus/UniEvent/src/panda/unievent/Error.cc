#include "Error.h"
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace panda { namespace unievent {

Error::Error (const string& what) : _what(what) {}

string        Error::_mkwhat () const         { return string(); }
const char*   Error::what    () const throw() { return whats().c_str(); }
const string& Error::whats   () const         { if (!_what) _what = _mkwhat(); return _what; }
Error*        Error::clone   () const         { return new Error(_what); }


ImplRequiredError::ImplRequiredError (const string& what) : Error(what + ": callback implementation required") {}

ImplRequiredError* ImplRequiredError::clone () const {
    auto ret = new ImplRequiredError({});
    ret->_what = _what;
    return ret;
}


errno_t CodeError::code () const { return _code; }
string  CodeError::name () const { return _code == 0 ? string() : string(uv_err_name(_code)); }
string  CodeError::str  () const { return _code == 0 ? string() : string(uv_strerror(_code)); }

string CodeError::_mkwhat () const {
    if (!_code) return {};
    return name() + '(' + string::from_number<errno_underlying_t>(code()) + ") " + str();
}

CodeError* CodeError::clone () const { return new CodeError(_code); }


string DyLibError::dlerror () const {
    return _code == 0 ? string() : string(uv_dlerror(lib));
}

string DyLibError::_mkwhat () const {
    if (!_code) return {};
    return CodeError::_mkwhat() + " : " + dlerror();
}

DyLibError* DyLibError::clone () const { return new DyLibError(_code, lib); }


SSLError::SSLError (int ssl_code) : CodeError(ERRNO_SSL), _ssl_code(ssl_code), _openssl_code(0) {
    if (_ssl_code == SSL_ERROR_SSL) {
        unsigned long tmp;
        while ((tmp = ERR_get_error())) _openssl_code = tmp;
    }
}

SSLError::SSLError (int ssl_code, unsigned long openssl_code) : CodeError(ERRNO_SSL), _ssl_code(ssl_code), _openssl_code(openssl_code) {}

int SSLError::ssl_code     () const { return _ssl_code; }
int SSLError::openssl_code () const { return _openssl_code; }

string SSLError::name () const { return "SSL"; }

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

string SSLError::str () const {
    if (_ssl_code != SSL_ERROR_SSL) return {};
    string ret(120);
    char* buf = ret.buf();
    ERR_error_string_n(_openssl_code, buf, ret.capacity());
    ret.length(strlen(buf));
    return ret;
}

SSLError* SSLError::clone () const { return new SSLError(_ssl_code, _openssl_code); }

}}
