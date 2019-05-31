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
    const LoopSP& loop () const { return _loop; }

    virtual const HandleType& type () const = 0;

    virtual bool active () const = 0;

    bool weak () const { return _weak; }

    void weak (bool value) {
        if (_weak == value) return;
        if (value) set_weak();
        else       unset_weak();
        _weak = value;
    }

    virtual void reset () = 0; // cancel everything in handle, leaving it in initial state, except for callbacks which is held
    virtual void clear () = 0; // full reset, return to initial state (as if handle has been just created via new())

    static const HandleType UNKNOWN_TYPE;

protected:
    friend Loop;
    using buf_alloc_fn = function<string(size_t cap)>;

    Handle () : _weak() { _ECTOR(); }
    Handle (const Handle&) = delete;

    ~Handle () {
        if (!_loop) return; // _init() has never been called (like exception in end class ctor)
        _loop->unregister_handle(this);
    }

    Handle& operator= (const Handle&) = delete;

    void _init (const LoopSP& loop) {
        _loop = loop;
        _loop->register_handle(this);
    }

    virtual void set_weak   () = 0;
    virtual void unset_weak () = 0;

private:
    LoopSP _loop;
    bool   _weak;
};

struct BHandle : Handle {
    using BackendHandle = backend::BackendHandle;

    bool active () const override { return _impl ? _impl->active() : false; }
    
    virtual void reset () = 0;
    virtual void clear () = 0;

protected:
    //friend Loop;

    mutable BackendHandle* _impl;

    BHandle () : _impl() { _ECTOR(); }

    ~BHandle () {
        if (_impl) _impl->destroy();
    }

    void _init (const LoopSP& loop, BackendHandle* impl = nullptr) {
        _impl = impl;
        Handle::_init(loop);
    }

    void set_weak   () override { impl()->set_weak(); }
    void unset_weak () override { impl()->unset_weak(); }

    virtual BackendHandle* new_impl () { abort(); }

    BackendHandle* impl () const {
        if (!_impl) {
            _impl = const_cast<BHandle*>(this)->new_impl();
            if (weak()) _impl->set_weak(); // preserve weak
        }
        return _impl;
    }
};
using BHandleSP = iptr<BHandle>;

inline void BHandle::reset () {
    if (!_impl) return;
    _impl->destroy();
    _impl = nullptr;
    if (weak()) _impl->set_weak(); // preserve weak
}

inline void BHandle::clear () {
    if (!_impl) return;
    _impl->destroy();
    _impl = nullptr;
}

}}
