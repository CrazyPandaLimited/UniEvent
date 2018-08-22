#include <panda/unievent/Async.h>
using namespace panda::unievent;

void Async::uvx_on_async (uv_async_t* handle) {
    Async* h = hcast<Async*>(handle);
    h->call_on_async();
}

void Async::send () {
    int err = uv_async_send(&uvh);
    if (err) throw AsyncError(err);
}

void Async::reset () {}

void Async::on_async () {
    if (async_callback) async_callback(this);
    else throw ImplRequiredError("Async::on_async");
}
