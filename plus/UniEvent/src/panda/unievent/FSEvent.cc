#include "FSEvent.h"
using namespace panda::unievent;

void FSEvent::uvx_on_fs_event (uv_fs_event_t* handle, const char* filename, int events, int) {
    FSEvent* h = hcast<FSEvent*>(handle);
    h->call_on_fs_event(filename, events);
}

void FSEvent::start (const char* path, int flags, fs_event_fn callback) {
    if (callback) fs_event.add(callback);
    int err = uv_fs_event_start(&uvh, uvx_on_fs_event, path, flags);
    if (err) throw CodeError(err);
}

void FSEvent::stop () {
    uv_fs_event_stop(&uvh);
}

void FSEvent::reset () {
    uv_fs_event_stop(&uvh);
}

void FSEvent::on_fs_event (const char* filename, int events) {
    if (fs_event.has_listeners()) fs_event(this, filename, events);
    else throw ImplRequiredError("FSEvent::on_fs_event");
}
