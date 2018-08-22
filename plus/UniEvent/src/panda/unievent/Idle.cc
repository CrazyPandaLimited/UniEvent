#include <panda/unievent/Idle.h>

namespace panda { namespace unievent {

void Idle::uvx_on_idle (uv_idle_t* handle) {
    Idle* h = hcast<Idle*>(handle);
    h->call_on_idle();
}

void Idle::on_idle () {
    if (idle_event.has_listeners()) idle_event(this);
    else throw ImplRequiredError("Idle::on_idle");
}

void Idle::start (idle_fn callback) {
    if (callback) idle_event.add(callback);
    uv_idle_start(&uvh, uvx_on_idle);
}

void Idle::stop  () { uv_idle_stop(&uvh); }
void Idle::reset () { uv_idle_stop(&uvh); }

}}
