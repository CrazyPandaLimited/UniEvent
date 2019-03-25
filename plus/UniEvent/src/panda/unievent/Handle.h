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

    bool active () const { return _impl->active(); }
    bool weak   () const { return _weak; }
    
    void weak (bool value) {
        if (_weak == value) return;
        if (value) _impl->set_weak();
        else       _impl->unset_weak();
        _weak = value;
    }

    virtual void reset () = 0;

    static const HandleType UNKNOWN_TYPE;

protected:
    friend Loop;
    using buf_alloc_fn = function<string(size_t cap)>;

    BackendHandle* _impl;

    Handle () : _weak(false) {
        _ECTOR();
    }

    ~Handle ();

    void _init (const LoopSP& loop, BackendHandle* impl) {
        _impl = impl;
        _loop = loop;
        _loop->register_handle(this);
    }

    optional<fd_t> fileno () const { return _impl->fileno(); }

    int  recv_buffer_size () const    { return _impl->recv_buffer_size(); }
    void recv_buffer_size (int value) { _impl->recv_buffer_size(value); }
    int  send_buffer_size () const    { return _impl->send_buffer_size(); }
    void send_buffer_size (int value) { _impl->send_buffer_size(value); }

private:
    LoopSP _loop;
    bool   _weak;
};

}}
