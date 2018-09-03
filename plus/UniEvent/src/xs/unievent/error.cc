#include <xs/unievent/error.h>
#include <typeinfo>
#include <cxxabi.h>
using namespace panda::unievent;
using namespace xs::unievent;
using xs::Stash;
using xs::Simple;

static const std::string cpp_errns  = "panda::unievent::";
static const std::string perl_errns = "UniEvent::";

const CodeError xs::unievent::code_error_def;

Stash xs::unievent::get_perl_class_for_err (const Error& err) {
    int status;
    char* cpp_class_name = abi::__cxa_demangle(typeid(err).name(), nullptr, nullptr, &status);
    if (status != 0) throw "[UniEvent] !critical! abi::__cxa_demangle error";
    if (strstr(cpp_class_name, cpp_errns.c_str()) != cpp_class_name) throw "[UniEvent] strange exception caught";
    std::string stash_name(perl_errns);
    stash_name.append(cpp_class_name + cpp_errns.length());
    free(cpp_class_name);
    Stash stash(std::string_view(stash_name.data(), stash_name.length()));
    if (!stash) throw Simple::format("[UniEvent] !critical! no error package: %s", stash_name.c_str());
    return stash;
}

Scalar xs::unievent::error_sv (const Error& err, bool with_mess) {
    const CodeError* cerr = panda::dyn_cast<const CodeError*>(&err);
    if (cerr && !*cerr) return Scalar::undef;

    auto stash = get_perl_class_for_err(err);

    auto args = Hash::create();
    args["what"] = Simple(err.what());

    if (with_mess) args["mess"] = mess(" ");

    if (cerr) {
        args["code"] = Simple((int)cerr->code());
        args["name"] = Simple(cerr->name());
        args["str"]  = Simple(cerr->str());
    }

    const SSLError* serr = panda::dyn_cast<const SSLError*>(&err);
    if (serr) {
        args["ssl_code"]     = Simple(serr->ssl_code());
        args["openssl_code"] = Simple(serr->openssl_code());
        args["library_str"]  = Simple(serr->library_str());
        args["function_str"] = Simple(serr->function_str());
        args["reason_str"]   = Simple(serr->reason_str());
        args["library"]      = Simple(serr->library());
        args["function"]     = Simple(serr->function());
        args["reason"]       = Simple(serr->reason());
    }

    Object ret = stash.call("new", { Ref::create(args) });
    if (!ret) throw Simple::format("[UniEvent] !critical! 'new' method in package %s returned non-object", stash.name().data());
    return ret.ref();
}
