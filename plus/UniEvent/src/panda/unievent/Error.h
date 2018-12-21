#pragma once
#include "inc.h"
#include <stdexcept>
#include <system_error>
#include <panda/lib.h>
#include <panda/string.h>
#include <panda/string_view.h>

namespace panda { namespace unievent {

using panda::string;
using std::string_view;

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

struct ImplRequiredError : Error {
    ImplRequiredError (const string& what);
    virtual ImplRequiredError* clone () const override;
};

struct CodeError : Error {
    CodeError () {}
    CodeError (const std::error_code& code, const string& name, const string& descr) : _code(code), _name(name), _descr(descr) {}

    const std::error_code& code () const;

    virtual string name  () const;
    virtual string descr () const;

    virtual CodeError* clone () const override;

    explicit
    operator bool () const { return _code.value(); }

    operator const CodeError* () const { return _code ? this : nullptr; }

protected:
    std::error_code _code;
    string          _name;
    string          _descr;
    string _mkwhat () const override;
};

//struct DyLibError : CodeError {
//    DyLibError (int code = 0, uv_lib_t* lib = nullptr) : CodeError(code), lib(lib) {}
//
//    virtual string dlerror () const;
//
//    string _mkwhat () const override;
//
//    virtual DyLibError* clone () const override;
//
//private:
//    uv_lib_t* lib;
//};

//struct SSLError : CodeError {
//    SSLError (int ssl_code);
//    SSLError (int ssl_code, unsigned long openssl_code);
//
//    int ssl_code     () const;
//    int openssl_code () const;
//
//    string name () const override;
//    int library  () const;
//    int function () const;
//    int reason   () const;
//
//    string library_str  () const;
//    string function_str () const;
//    string reason_str   () const;
//
//    string str () const override;
//
//    virtual SSLError* clone () const override;
//
//private:
//    int           _ssl_code;
//    unsigned long _openssl_code;
//};

}}
