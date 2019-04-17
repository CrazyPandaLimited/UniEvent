#pragma once
#include "Loop.h"
#include "Debug.h"
#include "Error.h"
#include "backend/BackendHandle.h"
#include <bitset>
#include <cstdint>
#include <panda/string.h>
#include <panda/lib/memory.h>

namespace panda { namespace unievent {

struct HandleType {
    const char* name;
    HandleType (const char* val) : name(val) {}
    bool operator== (const HandleType& oth) const { return name == oth.name; }
    bool operator<  (const HandleType& oth) const { return name < oth.name; }
};
std::ostream& operator<< (std::ostream& out, const HandleType&);

struct Handle : Refcnt, panda::lib::IntrusiveChainNode<Handle*> {
    using BackendHandle = backend::BackendHandle;

    const LoopSP& loop () const { return _loop; }

    virtual const HandleType& type () const = 0;

    bool active () const { return _impl ? _impl->active() : false; }
    bool weak   () const { return _weak; }
    
    void weak (bool value) {
        if (_weak == value) return;
        if (value) impl()->set_weak();
        else       impl()->unset_weak();
        _weak = value;
    }

    virtual void reset () = 0; // cancel everything in handle, leaving it in initial state, except for callbacks which is held
    virtual void clear () = 0; // full reset, return to initial state (as if handle has been just created via new())

    static const HandleType UNKNOWN_TYPE;

protected:
    friend Loop;
    using buf_alloc_fn = function<string(size_t cap)>;

    mutable BackendHandle* _impl;

    Handle () : _impl(), _weak(false) {
        _ECTOR();
    }

    ~Handle ();

    void _init (const LoopSP& loop, BackendHandle* impl = nullptr) {
        _impl = impl;
        _loop = loop;
        _loop->register_handle(this);
    }

    virtual BackendHandle* new_impl () { abort(); }

    BackendHandle* impl () const {
        if (!_impl) {
            _impl = const_cast<Handle*>(this)->new_impl();
            if (_weak) _impl->set_weak(); // preserve weak
        }
        return _impl;
    }

private:
    LoopSP _loop;
    bool   _weak;
};

inline void Handle::reset () {
    if (!_impl) return;
    _impl->destroy();
    _impl = nullptr;
    if (_weak) _impl->set_weak(); // preserve weak
}

inline void Handle::clear () {
    if (!_impl) return;
    _impl->destroy();
    _impl = nullptr;
}

}}
