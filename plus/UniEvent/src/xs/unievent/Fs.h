#pragma once
#include <xs.h>
#include <panda/unievent/Fs.h>

namespace xs {

template <> struct Typemap<panda::unievent::Fs::Stat> : TypemapBase<panda::unievent::Fs::Stat> {
    static Sv out (const panda::unievent::Fs::Stat&, const Sv& = Sv());
    static panda::unievent::Fs::Stat in (const Array&);
};

template <> struct Typemap<panda::unievent::Fs::DirEntry> : TypemapBase<panda::unievent::Fs::DirEntry> {
    static Sv out (const panda::unievent::Fs::DirEntry&, const Sv& = Sv());
    static panda::unievent::Fs::DirEntry in (const Array&);
};

template <class TYPE> struct Typemap<panda::unievent::Fs::Request*, TYPE> : TypemapObject<panda::unievent::Fs::Request*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref> {
    static panda::string package () { return "UniEvent::Fs::Request"; }
};

}
