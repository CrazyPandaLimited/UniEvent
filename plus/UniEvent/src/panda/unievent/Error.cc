#include <panda/unievent/Error.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace panda { namespace unievent {

Error::Error (const string& what) : _what(what) {}

string Error::_mkwhat () const throw() { return string(); }

const char*   Error::what  () const throw() { return whats().c_str(); }
const string& Error::whats () const throw() { if (!_what) _what = _mkwhat(); return _what; }

errno_t CodeError::code () const throw() { return _code; }
string  CodeError::name () const throw() { return _code == 0 ? string() : string(uv_err_name(_code)); }
string  CodeError::str  () const throw() { return _code == 0 ? string() : string(uv_strerror(_code)); }

string CodeError::_mkwhat () const throw() {
    if (!_code) return {};
    return name() + '(' + string::from_number<errno_underlying_t>(code()) + ") " + str();
}

ImplRequiredError::ImplRequiredError (const string& what) throw() : Error(what + ": callback implementation required") {}

string DyLibError::dlerror () const throw() {
    return _code == 0 ? string() : string(uv_dlerror(lib));
}

string DyLibError::_mkwhat () const throw() {
    if (!_code) return {};
    return CodeError::_mkwhat() + " : " + dlerror();
}

SSLError::SSLError (int ssl_code) throw() : StreamError(ERRNO_SSL), _ssl_code(ssl_code), _openssl_code(0) {
    if (_ssl_code == SSL_ERROR_SSL) {
        unsigned long tmp;
        while ((tmp = ERR_get_error())) _openssl_code = tmp;
    }
}

int SSLError::ssl_code     () const throw() { return _ssl_code; }
int SSLError::openssl_code () const throw() { return _openssl_code; }

string SSLError::name () const throw() { return string("SSL"); }

int SSLError::library  () const throw() { return _ssl_code == SSL_ERROR_SSL ? ERR_GET_LIB(_openssl_code) : 0; }
int SSLError::function () const throw() { return _ssl_code == SSL_ERROR_SSL ? ERR_GET_FUNC(_openssl_code) : 0; }
int SSLError::reason   () const throw() { return _ssl_code == SSL_ERROR_SSL ? ERR_GET_REASON(_openssl_code) : 0; }

string SSLError::library_str () const throw() {
    if (_ssl_code != SSL_ERROR_SSL) return string();
    const char* str = ERR_lib_error_string(_openssl_code);
    return str ? string(str) : string();
}

string SSLError::function_str () const throw() {
    if (_ssl_code != SSL_ERROR_SSL) return string();
    const char* str = ERR_func_error_string(_openssl_code);
    return str ? string(str) : string();
}

string SSLError::reason_str () const throw() {
    if (_ssl_code != SSL_ERROR_SSL) return string();
    const char* str = ERR_reason_error_string(_openssl_code);
    return str ? string(str) : string();
}

string SSLError::str () const throw() {
    if (_ssl_code != SSL_ERROR_SSL) return string();
    string ret(120);
    char* buf = ret.buf();
    ERR_error_string_n(_openssl_code, buf, ret.capacity());
    ret.length(strlen(buf));
    return ret;
}

}}
