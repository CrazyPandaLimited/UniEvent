#pragma once
#include "Loop.h"
#include "Debug.h"
#include "Error.h"
//#include "Command.h"
#include "backend/BackendHandle.h"
#include <bitset>
#include <cstdint>
#include <panda/string.h>
#include <panda/lib/memory.h>
#include <panda/CallbackDispatcher.h>

namespace panda { namespace unievent {

struct HandleType {
    const char* name;
    HandleType (const char* val) : name(val) {}
    bool operator== (const HandleType& oth) const { return name == oth.name; }
    bool operator<  (const HandleType& oth) const { return name < oth.name; }
};
std::ostream& operator<< (std::ostream& out, const HandleType&);

struct Handle : panda::lib::IntrusiveChainNode<Handle*>, Refcnt, panda::lib::AllocatedObject<Handle> {
    using BackendHandle = backend::BackendHandle;
    using buf_alloc_fn  = panda::function<string(Handle* h, size_t cap)>;

    buf_alloc_fn buf_alloc_callback;

    Handle () : _weak(false) /*, in_user_callback(false)*/ {
        _ECTOR();
        //asyncq.head = asyncq.tail = nullptr;
    }

    void release () const {
        if (refcnt() <= 1 && _impl) const_cast<Handle*>(this)->destroy();
        Refcnt::release();
    }

    virtual void destroy ();

    Loop* loop () const { return _impl ? _impl->loop()->frontend() : nullptr; }

    virtual const HandleType& type () const = 0;

    bool active () const { return _impl ? _impl->active() : false; }
    bool weak   () const { return _weak; }
    
    void weak (bool value) {
        if (_weak == value) return;
        if (value) impl()->set_weak();
        else       impl()->unset_weak();
        _weak = value;
    }

//    os_fd_t fileno () const {
//        os_fd_t fd;
//        int err = uv_fileno(uvhp, &fd);
//        if (err) throw CodeError(err);
//        return fd;
//    }

    virtual string buf_alloc (size_t cap);

    virtual void reset () = 0;

//    bool async_locked  () const { return flags & HF_BUSY; }
//    bool asyncq_empty  () const { return !asyncq.head; }
//
//    void async_lock   () {
//        _EDEBUG("lock");
//        flags |= HF_BUSY;
//    }
//
//    void async_unlock () {
//        _EDEBUG("unlock");
//        async_unlock_noresume();
//        asyncq_resume();
//    }
//
//    void asyncq_run (CommandBase* cmd) {
//        if (async_locked()) asyncq_push(cmd);
//        else {
//            cmd->handle = this;
//            cmd->run();
//            delete cmd;
//        }
//    }
//
//    void asyncq_run (CommandCallback::command_cb cb) {
//        asyncq_run(new CommandCallback(cb));
//    }
//
//    friend struct CommandCloseDelete;
//    friend struct CommandCloseReinit;

    static const HandleType UNKNOWN_TYPE;

protected:

//    bool         in_user_callback;

    void _init (BackendHandle* impl) {
        _impl = impl;
        loop()->register_handle(this);
    }

    BackendHandle* impl () const {
        if (!_impl) throw Error("Loop has been destroyed and this handle can not be used anymore");
        return _impl;
    }

//    struct InUserCallbackLock {
//        Handle* h;
//        InUserCallbackLock(Handle* h) : h(h){
//            h->in_user_callback = true;
//        }
//        ~InUserCallbackLock() {
//            h->in_user_callback = false;
//        }
//    };
//
//    InUserCallbackLock lock_in_callback() {
//        return InUserCallbackLock(this);
//    }
//
//    struct {
//        CommandBase* head;
//        CommandBase* tail;
//    } asyncq;
//
//    void _init (void* hptr) {
//        uvhp = static_cast<uv_handle_t*>(hptr);
//        uvhp->data = this;
//    }
//
//    virtual void on_handle_reinit ();
//
//    virtual void close_reinit (bool keep_asyncq = false);
//
//    void asyncq_cancel ();
//    void _asyncq_cancel ();
//
//    void asyncq_push (CommandBase* cmd) {
//        cmd->handle = this;
//        cmd->next = nullptr;
//        if (asyncq.head) asyncq.tail->next = cmd;
//        else asyncq.head = cmd;
//        asyncq.tail = cmd;
//    }
//
//    void async_unlock_noresume () {
//        //assert(flags & HF_BUSY); // COMMENTED OUT - TO REFACTOR - ????? ?????? ????????? ? ?????? Front ? ?????? ????????? ?????? ? reset().
//                                   // ????? ????????? ? UniEvent 2.0 (any-engine) - ????????????????? ???????
//        flags &= ~HF_BUSY;
//    }
//
//    void asyncq_resume () {
//        assert(!async_locked());
//        while (asyncq.head && !async_locked()) {
//            CommandBase* cmd = asyncq.head;
//            asyncq.head = cmd->next;
//            cmd->run();
//            delete cmd;
//        }
//        if (!asyncq.head) asyncq.tail = nullptr;
//    }
//
//    void set_recv_buffer_size(int value) {
//        uv_recv_buffer_size(uvhp, &value);
//    }
//
//    void set_send_buffer_size(int value) {
//        uv_send_buffer_size(uvhp, &value);
//    }
//
    // private dtor prevents creating Handles on the stack / statically / etc.
    virtual ~Handle () {}

public:
//    // clang restricts friendliness forwarding via derived classes as of example in 11.4
//    // so making it public: https://bugs.llvm.org/show_bug.cgi?id=6840
//    static void uvx_on_buf_alloc (uv_handle_t* handle, size_t size, uv_buf_t* uvbuf);
//
//    virtual void _close(); // ignores command queue, calls uv_close, beware using this

private:
    BackendHandle* _impl;
    bool           _weak;

//    void close_delete ();
//
//    static void uvx_on_close_delete (uv_handle_t* handle);
//    static void uvx_on_close_reinit (uv_handle_t* handle);
};

inline void refcnt_dec (const Handle* o) { o->release(); }

}}
