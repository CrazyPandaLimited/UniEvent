#include "unievent.h"
#include "unievent/Listener.h"
#include <typeinfo>
#include <cxxabi.h>

using namespace panda::unievent;
using panda::string;
using panda::string_view;

namespace xs { namespace unievent {

Sv::payload_marker_t event_listener_marker;

static bool _init () {
    event_listener_marker.svt_free = [](pTHX_ SV*, MAGIC* mg) -> int {
        delete (XSListener*)mg->mg_ptr;
        return 0;
    };
    return true;
}
static bool __init = _init();

Stash get_perl_class_for_err (const Error& err) {
    int status;
    char* cpp_class_name = abi::__cxa_demangle(typeid(err).name(), nullptr, nullptr, &status);
    if (status != 0) throw "[UniEvent] !critical! abi::__cxa_demangle error";
    static const std::string cpp_errns  = "panda::unievent::";
    if (strstr(cpp_class_name, cpp_errns.c_str()) != cpp_class_name) throw "[UniEvent] strange exception caught";
    std::string stash_name("UniEvent::");
    stash_name.append(cpp_class_name + cpp_errns.length());
    free(cpp_class_name);
    Stash stash(string_view(stash_name.data(), stash_name.length()));
    if (!stash) throw Simple::format("[UniEvent] !critical! no error package: %s", stash_name.c_str());
    return stash;
}

string sv2buf (const Sv& sv) {
    string buf;
    if (sv.is_array_ref()) { // [$str1, $str2, ...]
        Array wlist(sv);
        STRLEN sum = 0;
        for (auto it = wlist.cbegin(); it != wlist.cend(); ++it) {
            STRLEN len;
            SvPV(*it, len);
            sum += len;
        }
        if (!sum) return string();

        char* ptr = buf.reserve(sum);
        for (auto it = wlist.cbegin(); it != wlist.cend(); ++it) {
            STRLEN len;
            const char* data = SvPV(*it, len);
            memcpy(ptr, data, len);
            ptr += len;
        }
        buf.length(sum);
    } else { // $str
        STRLEN len;
        const char* data = SvPV(sv, len);
        if (!len) return string();
        buf.assign(data, len);
    }
    return buf;
}

void XSListener::_throw_noobj (const Simple& evname) {
    auto err = std::string("Handle needs event listener for event '") + evname.c_str() + "' but listener was weak and went out of scope";
    throw std::logic_error(err);
}

}}

namespace xs {

static inline void throw_bad_hints () { throw "argument is not a valid AddrInfoHints"; }

AddrInfoHints Typemap<AddrInfoHints>::in (SV* arg) {
    if (!SvOK(arg)) return AddrInfoHints();

    if (SvPOK(arg)) {
        if (SvCUR(arg) < sizeof(AddrInfoHints)) throw_bad_hints();
        return *reinterpret_cast<AddrInfoHints*>(SvPVX(arg));
    }

    if (!Sv(arg).is_hash_ref()) throw_bad_hints();
    AddrInfoHints ret;
    Hash h = arg;
    for (auto& row : h) {
        auto k = row.key();
        auto val = Simple(row.value());
        if      (k == "family"  ) ret.family   = val;
        else if (k == "socktype") ret.socktype = val;
        else if (k == "protocol") ret.protocol = val;
        else if (k == "flags"   ) ret.flags    = val;
    }
    return ret;
}

Sv Typemap<Fs::Stat>::out (const Fs::Stat& s, const Sv&) {
    return Ref::create(Array::create({
        Simple(s.dev),
        Simple(s.ino),
        Simple(s.mode),
        Simple(s.nlink),
        Simple(s.uid),
        Simple(s.gid),
        Simple(s.rdev),
        Simple(s.size),
        Simple(s.atime.get()),
        Simple(s.mtime.get()),
        Simple(s.ctime.get()),
        Simple(s.blksize),
        Simple(s.blocks),
        Simple(s.flags),
        Simple(s.gen),
        Simple(s.birthtime.get()),
        Simple((int)s.type()),
        Simple(s.perms()),
    }));
}

Fs::Stat Typemap<Fs::Stat>::in (const Array& a) {
    Fs::Stat ret;
    ret.dev       = Simple(a[0]);
    ret.ino       = Simple(a[1]);
    ret.mode      = Simple(a[2]);
    ret.nlink     = Simple(a[3]);
    ret.uid       = Simple(a[4]);
    ret.gid       = Simple(a[5]);
    ret.rdev      = Simple(a[6]);
    ret.size      = Simple(a[7]);
    ret.atime     = Simple(a[8]);
    ret.mtime     = Simple(a[9]);
    ret.ctime     = Simple(a[10]);
    ret.blksize   = Simple(a[11]);
    ret.blocks    = Simple(a[12]);
    ret.flags     = Simple(a[13]);
    ret.gen       = Simple(a[14]);
    ret.birthtime = Simple(a[15]);
    return ret;
}

Sv Typemap<Fs::DirEntry>::out (const Fs::DirEntry& de, const Sv&) {
    return Ref::create(Array::create({
        Simple(de.name()),
        Simple((int)de.type())
    }));
}

Fs::DirEntry Typemap<Fs::DirEntry>::in (const Array& a) {
    return Fs::DirEntry(Simple(a[0]).as_string(), (Fs::FileType)(int)Simple(a[1]));
}

}
