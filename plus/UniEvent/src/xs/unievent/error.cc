#include "error.h"
#include <typeinfo>
#include <cxxabi.h>
using namespace panda::unievent;
using namespace xs::unievent;
using xs::Stash;
using xs::Simple;

static const std::string cpp_errns  = "panda::unievent::";
static const std::string perl_errns = "UniEvent::";

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
