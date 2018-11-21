#pragma once

#include <bitset>
#include <cstdint>

#include <panda/CallbackDispatcher.h>

#include "global.h"
#include "Loop.h"
#include "Debug.h"
#include "Command.h"

namespace panda { namespace unievent {

typedef uv_os_fd_t os_fd_t;

typedef enum {
    HTYPE_UNKNOWN_HANDLE = UV_UNKNOWN_HANDLE,
#define XX(uc,lc) HTYPE_##uc,
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    HTYPE_FILE = UV_FILE,
    HTYPE_MAX  = UV_HANDLE_TYPE_MAX
} handle_type;

struct Handle : virtual Refcnt {
    typedef panda::function<string(Handle* h, size_t cap)> buf_alloc_fn;

    buf_alloc_fn buf_alloc_event;

    void release () const = delete;
    void release () {
        if (refcnt() <= 1) {
            close_delete();
        }
        else Refcnt::release();
    }

    Loop*       loop   () const { return static_cast<Loop*>(uvhp->loop->data); }
    handle_type type   () const { return static_cast<handle_type>(uvhp->type); }
    bool        active () const { return uv_is_active(uvhp); }
    bool        weak   () const { return flags & HF_WEAK; }
    
    os_fd_t fileno () const {
        os_fd_t fd;
        int err = uv_fileno(uvhp, &fd);
        if (err) throw CodeError(err);
        return fd;
    }

    void weak (bool value) {
        if (weak() == value) return;
        if (value) {
            uv_unref(uvhp);
            flags |= HF_WEAK;
        } else  {
            uv_ref(uvhp);
            flags &= ~HF_WEAK;
        }
    }

    virtual string buf_alloc (size_t cap);

    void swap (Handle* other);

    virtual void reset () = 0;

    bool async_locked  () const { return flags & HF_BUSY; }
    bool asyncq_empty  () const { return !asyncq.head; }

    void async_lock   () { 
        _EDEBUG("lock");
        flags |= HF_BUSY; 
    }

    void async_unlock () {
        _EDEBUG("unlock");
        async_unlock_noresume();
        asyncq_resume();
    }

    void asyncq_run (CommandBase* cmd) {
        if (async_locked()) asyncq_push(cmd);
        else {
            cmd->handle = this;
            cmd->run();
            delete cmd;
        }
    }

    void asyncq_run (CommandCallback::command_cb cb) {
        asyncq_run(new CommandCallback(cb));
    }

    static handle_type guess_type (file_t file);

    friend struct CommandCloseDelete;
    friend struct CommandCloseReinit;

protected:
    uv_handle_t* uvhp;
    uint32_t     flags;
    bool         in_user_callback;

    struct InUserCallbackLock {
        Handle* h;
        InUserCallbackLock(Handle* h) : h(h){
            h->in_user_callback = true;
        }
        ~InUserCallbackLock() {
            h->in_user_callback = false;
        }
    };

    InUserCallbackLock lock_in_callback() {
        return InUserCallbackLock(this);
    }

    struct {
        CommandBase* head;
        CommandBase* tail;
    } asyncq;

    static const uint32_t HF_WEAK = 0x01;
    static const uint32_t HF_BUSY = 0x02;
    static const uint32_t HF_LAST = HF_BUSY;

    Handle () : flags(0), in_user_callback(false) {
        _ECTOR();
        asyncq.head = asyncq.tail = nullptr;
    }

    void _init (void* hptr) {
        uvhp = static_cast<uv_handle_t*>(hptr);
        uvhp->data = this;
    }

    virtual void on_handle_reinit ();

    virtual void close_reinit (bool keep_asyncq = false);

    void asyncq_cancel ();
    void _asyncq_cancel ();

    void asyncq_push (CommandBase* cmd) {
        cmd->handle = this;
        cmd->next = nullptr;
        if (asyncq.head) asyncq.tail->next = cmd;
        else asyncq.head = cmd;
        asyncq.tail = cmd;
    }

    void async_unlock_noresume () {
        //assert(flags & HF_BUSY); // COMMENTED OUT - TO REFACTOR - Такие случаи возникают в тестах Front с учетом обнуления флагов в reset().
                                   // нужно исправить в UniEvent 2.0 (any-engine) - перепроектировать очереди
        flags &= ~HF_BUSY;
    }

    void asyncq_resume () {
        assert(!async_locked());
        while (asyncq.head && !async_locked()) {
            CommandBase* cmd = asyncq.head;
            asyncq.head = cmd->next;
            cmd->run();
            delete cmd;
        }
        if (!asyncq.head) asyncq.tail = nullptr;
    }
   
    void set_recv_buffer_size(int value) {
        uv_recv_buffer_size(uvhp, &value);
    }
    
    void set_send_buffer_size(int value) {
        uv_send_buffer_size(uvhp, &value);
    }

    // private dtor prevents creating Handles on the stack / statically / etc.
    virtual ~Handle ();

public:
    // clang restricts friendliness forwarding via derived classes as of example in 11.4
    // so making it public: https://bugs.llvm.org/show_bug.cgi?id=6840
    static void uvx_on_buf_alloc (uv_handle_t* handle, size_t size, uv_buf_t* uvbuf);
    
    virtual void _close(); // ignores command queue, calls uv_close, beware using this

private:
    void close_delete ();

    static void uvx_on_close_delete (uv_handle_t* handle);
    static void uvx_on_close_reinit (uv_handle_t* handle);
};

void refcnt_dec (const Handle* o); // no body for startup error - cannot dec const object
inline void refcnt_dec (Handle* o) { o->release(); }

}}
