#pragma once
#include <stdexcept>
#include <panda/lib.h>
#include <panda/string.h>
#include <panda/unievent/inc.h>

namespace panda { namespace unievent {

using panda::string;
class Loop;

class Error {
public:
    Error () {}
    Error (const string& what_arg);
    Error (const char* what_arg);
    virtual string what () const throw();
private:
    string _what;
};

class CodeError : public Error {
public:
    CodeError (uv_errno_t code) throw() : Error(), _code(static_cast<errno_t>(code)) {}
    CodeError (int code = 0)    throw() : Error(), _code(static_cast<errno_t>(code)) {}
    CodeError (errno_t code)    throw() : Error(), _code(code) {}

    virtual errno_t code () const throw();
    virtual string  name () const throw();
    virtual string  str  () const throw();

    string what () const throw() override;

    explicit
    operator bool() const { return _code != 0; }

protected:
    errno_t _code;
};

class ImplRequiredError : public Error {
public:
    ImplRequiredError (const string& what) throw();
};

class DyLibError : public CodeError {
public:
    DyLibError (int code = 0, uv_lib_t* lib = nullptr) throw() : CodeError(code), lib(lib) {}

    virtual string dlerror () const throw();

    string what () const throw() override;

private:
    uv_lib_t* lib;
};

#define _PE_CODE_ERROR_CLASS(cls,base) \
    class cls : public base { public: \
        cls (uv_errno_t code) throw() : base(code) {} \
        cls (int code = 0)    throw() : base(code) {} \
        cls (errno_t code)    throw() : base(code) {} \
    }

#define _PE_HANDLE_ERROR_CLASS(cls) _PE_CODE_ERROR_CLASS(cls,HandleError)

_PE_CODE_ERROR_CLASS(RequestError,CodeError);
_PE_CODE_ERROR_CLASS(LoopError,CodeError);
_PE_CODE_ERROR_CLASS(HandleError,CodeError);
_PE_CODE_ERROR_CLASS(OperationError,CodeError);

_PE_HANDLE_ERROR_CLASS(AsyncError);
_PE_HANDLE_ERROR_CLASS(CheckError);
_PE_HANDLE_ERROR_CLASS(FSEventError);
_PE_HANDLE_ERROR_CLASS(FSPollError);
_PE_HANDLE_ERROR_CLASS(FSRequestError);
_PE_HANDLE_ERROR_CLASS(PollError);
_PE_HANDLE_ERROR_CLASS(ProcessError);
_PE_HANDLE_ERROR_CLASS(SignalError);
_PE_HANDLE_ERROR_CLASS(TimerError);
_PE_HANDLE_ERROR_CLASS(UDPError);
_PE_HANDLE_ERROR_CLASS(StreamError);
_PE_CODE_ERROR_CLASS(PipeError,StreamError);
_PE_CODE_ERROR_CLASS(TCPError,StreamError);
_PE_CODE_ERROR_CLASS(TTYError,StreamError);
_PE_CODE_ERROR_CLASS(ThreadError, RequestError);
_PE_CODE_ERROR_CLASS(ResolveError,CodeError);
_PE_CODE_ERROR_CLASS(WorkError,CodeError);

#undef _PE_CODE_ERROR_CLASS
#undef _PE_HANDLE_ERROR_CLASS
#undef _PE_REQUEST_ERROR_CLASS

class SSLError : public StreamError {
public:
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
