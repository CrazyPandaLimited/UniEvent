#include <panda/unievent/FSPoll.h>
using namespace panda::unievent;

void FSPoll::uvx_on_fs_poll (uv_fs_poll_t* handle, int errcode, const stat_t* prev, const stat_t* curr) {
    FSPoll* h = hcast<FSPoll*>(handle);
    FSPollError err(errcode);
    h->call_on_fs_poll(prev, curr, err);
}

void FSPoll::on_fs_poll (const stat_t* prev, const stat_t* curr, const FSPollError& err) {
    if (fs_poll_event.has_listeners()) fs_poll_event(this, prev, curr, err);
    else throw ImplRequiredError("FSPoll::on_fs_poll");
}

void FSPoll::start (const char* path, unsigned int interval, fs_poll_fn callback) {
    if (callback) fs_poll_event.add(callback);
    int err = uv_fs_poll_start(&uvh, uvx_on_fs_poll, path, interval);
    if (err) throw FSPollError(err);
}

void FSPoll::stop () {
    uv_fs_poll_stop(&uvh);
}

void FSPoll::reset () {
    uv_fs_poll_stop(&uvh);
}
