#include "Thread.h"
using namespace panda::unievent;

thread_local thread_t Thread::_thread_self = 0;

void Thread::uvx_on_create (void* arg) {
    thread_ctx_t* ctx = reinterpret_cast<thread_ctx_t*>(arg);
    Thread* h = ctx->myself;
    h->on_create(ctx->arg);
}

void Thread::create (void* arg) {
    ctx.arg = arg;
    int err = uv_thread_create(&handle, uvx_on_create, &ctx);
    if (err) throw CodeError(err);
}

void Thread::join () {
    int err = uv_thread_join(&handle);
    if (err) throw CodeError(err);
}

void Thread::on_create (void* arg) {
    if (create_callback) create_callback(this, arg);
    else throw ImplRequiredError("Thread::on_create");
}
