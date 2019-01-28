#include "Handle.h"
namespace panda { namespace unievent {

static const size_t MIN_ALLOC_SIZE = 1024;

const HandleType Handle::UNKNOWN_TYPE("unknown");

string Handle::buf_alloc (size_t cap) {
    if (buf_alloc_callback) return buf_alloc_callback(this, cap);
    string ret(cap);
    return ret;
}

void Handle::destroy () {
    _EDEBUGTHIS("%s, locked=%d", type().name, /*async_locked()*/0);
//    if (async_locked()) {
//        asyncq_push(new CommandCloseDelete());
//        return;
//    }
//    if (!asyncq_empty()) asyncq_cancel();
    loop()->unregister_handle(this);
    _impl->destroy();
    _impl = nullptr;
}

//void Handle::uvx_on_buf_alloc (uv_handle_t* handle, size_t size, uv_buf_t* uvbuf) {
//    if (size < MIN_ALLOC_SIZE) size = MIN_ALLOC_SIZE;
//    Handle* h = static_cast<Handle*>(handle->data);
//    string buf = h->buf_alloc(size);
//    char* ptr = buf.shared_buf();
//    auto  cap = buf.shared_capacity();
//
//    assert(cap >= size);
//
//    size_t align = (size_t(ptr) + cap) % sizeof(void*);
//    cap -= align;
//
//    auto availcap = cap - sizeof(string);
//
//    new ((string*)(ptr + availcap)) string(buf); // save string object at the end of the buffer, keeping it's internal ptr alive
//
//    uvbuf->base = ptr;
//    uvbuf->len  = availcap;
//}
//
//void Handle::uvx_on_close_delete (uv_handle_t* handle) {
//    Handle* h = static_cast<Handle*>(handle->data);
//    _EDEBUG("[%p] uvx_on_close_delete", h);
//    delete h;
//}
//
//void Handle::uvx_on_close_reinit (uv_handle_t* handle) {
//    Handle* h = static_cast<Handle*>(handle->data);
//    _EDEBUG("[%p] uvx_on_close_reinit", h);
//    h->on_handle_reinit();
//    h->async_unlock();
//    h->release();
//}
//void Handle::close_reinit (bool keep_asyncq) {
//    _EDEBUGTHIS("close_reinit, keep_asyncq: %d, in_user_callback: %d, asyncq_empty(): %d ", keep_asyncq, in_user_callback, asyncq_empty());
//    if (!keep_asyncq && in_user_callback) {
//        if (!asyncq_empty()) _asyncq_cancel(); // do not lock and unlock here
//        keep_asyncq = true;
//    }
//    if (keep_asyncq) {
//        if (async_locked()) {
//            asyncq_push(new CommandCloseReinit());
//            return;
//        }
//    }
//    else if (!asyncq_empty()) asyncq_cancel();
//    _close();
//}
//
//void Handle::_close() {
//    if (!uv_is_closing(uvhp)) {
//        async_lock();
//        uv_close(uvhp, uvx_on_close_reinit);
//        retain();
//    }
//}
//
//void Handle::asyncq_cancel () {
//    async_lock();
//    _asyncq_cancel();
//    async_unlock(); // will trigger commands if cancel callbacks created anything
//}
//
//void Handle::_asyncq_cancel() {
//    CommandBase* cmd = asyncq.head;
//    asyncq.head = asyncq.tail = nullptr;
//    while (cmd) {
//        cmd->cancel();
//        CommandBase* next_cmd = cmd->next;
//        delete cmd;
//        cmd = next_cmd;
//    }
//}

//void Handle::on_handle_reinit () {
//    _EDEBUGTHIS("on_handle_reinit");
//    flags = (flags & HF_BUSY) ? HF_BUSY : 0;
//}

std::ostream& operator<< (std::ostream& out, const HandleType& type) {
    return out << type.name;
}

}}
