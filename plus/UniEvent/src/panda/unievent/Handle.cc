#include "Handle.h"
using namespace panda::unievent;

static const size_t MIN_ALLOC_SIZE = 1024;

void Handle::uvx_on_buf_alloc (uv_handle_t* handle, size_t size, uv_buf_t* uvbuf) {
    if (size < MIN_ALLOC_SIZE) size = MIN_ALLOC_SIZE;
    Handle* h = static_cast<Handle*>(handle->data);
    string buf = h->buf_alloc(size);
    char* ptr = buf.shared_buf();
    auto  cap = buf.shared_capacity();

    assert(cap >= size);

    size_t align = (size_t(ptr) + cap) % sizeof(void*);
    cap -= align;

    auto availcap = cap - sizeof(string);

    new ((string*)(ptr + availcap)) string(buf); // save string object at the end of the buffer, keeping it's internal ptr alive

    uvbuf->base = ptr;
    uvbuf->len  = availcap;
}

void Handle::uvx_on_close_delete (uv_handle_t* handle) {
    Handle* h = static_cast<Handle*>(handle->data);
    _EDEBUG("[%p] uvx_on_close_delete", h);
    delete h;
}

void Handle::uvx_on_close_reinit (uv_handle_t* handle) {
    Handle* h = static_cast<Handle*>(handle->data);
    _EDEBUG("[%p] uvx_on_close_reinit", h);
    h->on_handle_reinit();
    h->async_unlock();
    h->release();
}

void Handle::swap (Handle *other) {
    assert(uvhp->type == other->uvhp->type);
    assert(uvhp->loop == other->uvhp->loop);

    if (this == other) return;
    size_t size = uv_handle_size(uvhp->type);

    std::swap_ranges((char*) uvhp, (char*)(uvhp) + size, (char*) other->uvhp); // could be twice faster by using uint64_t instead of char
    std::swap(uvhp->data, other->uvhp->data);
}

handle_type Handle::guess_type (file_t file) {
    uv_handle_type type = uv_guess_handle(file);
    return static_cast<handle_type>(type);
}

void Handle::close_delete () {
    _EDEBUGTHIS("close_delete, locked: %d, type: %d", async_locked(), type());
    if (async_locked()) {
        asyncq_push(new CommandCloseDelete());
        return;
    }
    if (!asyncq_empty()) asyncq_cancel();
    assert(!uv_is_closing(uvhp));
    uv_close(uvhp, uvx_on_close_delete);
}

void Handle::close_reinit (bool keep_asyncq) {
    _EDEBUGTHIS("close_reinit, keep_asyncq: %d, in_user_callback: %d, asyncq_empty(): %d ", keep_asyncq, in_user_callback, asyncq_empty());
    if (!keep_asyncq && in_user_callback) {
        if (!asyncq_empty()) _asyncq_cancel(); // do not lock and unlock here
        keep_asyncq = true;
    }
    if (keep_asyncq) {
        if (async_locked()) {
            asyncq_push(new CommandCloseReinit());
            return;
        }
    }
    else if (!asyncq_empty()) asyncq_cancel();
    _close();
}

void Handle::_close() {
    if (!uv_is_closing(uvhp)) {
        async_lock();
        uv_close(uvhp, uvx_on_close_reinit);
        retain();
    }
}

void Handle::asyncq_cancel () {
    async_lock();
    _asyncq_cancel();
    async_unlock(); // will trigger commands if cancel callbacks created anything
}

void Handle::_asyncq_cancel() {
    CommandBase* cmd = asyncq.head;
    asyncq.head = asyncq.tail = nullptr;
    while (cmd) {
        cmd->cancel();
        CommandBase* next_cmd = cmd->next;
        delete cmd;
        cmd = next_cmd;
    }
}

Handle::~Handle() { _EDTOR(); }

string Handle::buf_alloc (size_t cap) {
    if (buf_alloc_event) return buf_alloc_event(this, cap);
    string ret(cap);
    return ret;
}

void Handle::on_handle_reinit () {
    _EDEBUGTHIS("on_handle_reinit");
    flags = (flags & HF_BUSY) ? HF_BUSY : 0;
}
