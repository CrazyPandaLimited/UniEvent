#include <panda/unievent/Poll.h>
using namespace panda::unievent;

void Poll::uvx_on_poll (uv_poll_t* handle, int status, int events) {
    Poll* h = hcast<Poll*>(handle);
    CodeError err(status < 0 ? status : 0);
    h->call_on_poll(events, err);
}

void Poll::on_poll (int events, const CodeError& err) {
    if (poll_event.has_listeners()) poll_event(this, events, err);
    else throw ImplRequiredError("Poll::on_poll");
}

void Poll::start (int events, poll_fn callback) {
    if (callback) poll_event.add(callback);
    int err = uv_poll_start(&uvh, events, uvx_on_poll);
    if (err) throw CodeError(err);
}

void Poll::stop () {
    int err = uv_poll_stop(&uvh);
    if (err) throw CodeError(err);
}

void Poll::reset () { stop(); }
