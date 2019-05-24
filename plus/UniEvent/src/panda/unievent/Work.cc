#include "Work.h"
using namespace panda::unievent;

WorkSP Work::queue (const work_fn& wcb, const after_work_fn& awcb, const LoopSP& loop) {
    WorkSP ret = new Work(loop);
    ret->work_cb = wcb;
    ret->after_work_cb = awcb;
    ret->queue();
    return ret;
}

void Work::queue () {
    _loop->register_work(this);
    impl()->queue();
    _active = true;
}

bool Work::cancel () {
    if (!_active) return true;
    if (!_impl->destroy()) return false;
    _impl = nullptr;
    handle_after_work(CodeError(std::errc::operation_canceled));
    return true;
}

void Work::on_work () {
    work_cb(this);
}

void Work::on_after_work (const CodeError& err) {
    if (after_work_cb) after_work_cb(this, err);
}

void Work::handle_work () {
    on_work();
}

void Work::handle_after_work (const CodeError& err) {
    WorkSP self = this; // hold
    _loop->unregister_work(self);
    _active = false;
    on_after_work(err);
}
