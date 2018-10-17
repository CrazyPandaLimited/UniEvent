#include "Check.h"
using namespace panda::unievent;

void Check::uvx_on_check (uv_check_t* handle) {
    Check* h = hcast<Check*>(handle);
    h->call_on_check();
}

void Check::start (check_fn callback) {
    if (callback) check_event.add(callback);
    uv_check_start(&uvh, uvx_on_check);
}

void Check::stop () {
    uv_check_stop(&uvh);
}

void Check::reset () {
    uv_check_stop(&uvh);
}

void Check::on_check () {
    if (check_event.has_listeners()) check_event(this);
    else throw ImplRequiredError("Check::on_check");
}
