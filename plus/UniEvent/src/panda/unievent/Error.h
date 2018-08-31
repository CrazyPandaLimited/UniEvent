#pragma once
#include <stdexcept>
#include <panda/lib.h>
#include <panda/string.h>
#include <panda/string_view.h>
#include <panda/unievent/inc.h>

namespace panda { namespace unievent {

using panda::string;
using std::string_view;
class Loop;

struct Error : std::exception {
    Error () {}
    Error (const string&);
    virtual const char*   what  () const throw() override;
    virtual const string& whats () const throw();
protected:
    mutable string _what;
    virtual string _mkwhat () const throw();
};

struct CodeError : Error {
    CodeError (uv_errno_t code) throw() : CodeError(static_cast<errno_t>(code)) {}
    CodeError (int code = 0)    throw() : CodeError(static_cast<errno_t>(code)) {}
    CodeError (errno_t code)    throw() : Error(), _code(code)                  {}

    virtual errno_t code () const throw();
    virtual string  name () const throw();
    virtual string  str  () const throw();

    explicit
    operator bool() const { return _code != 0; }

protected:
    errno_t _code;
    string _mkwhat () const throw() override;
};

struct ImplRequiredError : Error {
    ImplRequiredError (const string& what) throw();
};

struct DyLibError : CodeError {
    DyLibError (int code = 0, uv_lib_t* lib = nullptr) throw() : CodeError(code), lib(lib) {}

    virtual string dlerror () const throw();

    string _mkwhat () const throw() override;

private:
    uv_lib_t* lib;
};

struct RequestError   : CodeError    { using CodeError::CodeError; };
struct LoopError      : CodeError    { using CodeError::CodeError; };
struct HandleError    : CodeError    { using CodeError::CodeError; };
struct OperationError : CodeError    { using CodeError::CodeError; };
struct ResolveError   : CodeError    { using CodeError::CodeError; };
struct WorkError      : CodeError    { using CodeError::CodeError; };
struct ThreadError    : RequestError { using RequestError::RequestError; };
struct AsyncError     : HandleError  { using HandleError::HandleError; };
struct CheckError     : HandleError  { using HandleError::HandleError; };
struct FSEventError   : HandleError  { using HandleError::HandleError; };
struct FSPollError    : HandleError  { using HandleError::HandleError; };
struct FSRequestError : HandleError  { using HandleError::HandleError; };
struct PollError      : HandleError  { using HandleError::HandleError; };
struct ProcessError   : HandleError  { using HandleError::HandleError; };
struct SignalError    : HandleError  { using HandleError::HandleError; };
struct TimerError     : HandleError  { using HandleError::HandleError; };
struct UDPError       : HandleError  { using HandleError::HandleError; };
struct StreamError    : HandleError  { using HandleError::HandleError; };
struct PipeError      : StreamError  { using StreamError::StreamError; };
struct TCPError       : StreamError  { using StreamError::StreamError; };
struct TTYError       : StreamError  { using StreamError::StreamError; };

struct SSLError : StreamError {
    SSLError (int ssl_code) throw();

    int ssl_code     () const throw();
    int openssl_code () const throw();

    string name () const throw() override;

    int library  () const throw();
    int function () const throw();
    int reason   () const throw();

    string library_str  () const throw();
    string function_str () const throw();
    string reason_str   () const throw();

    string str () const throw() override;

private:
    int           _ssl_code;
    unsigned long _openssl_code;
};

}}
