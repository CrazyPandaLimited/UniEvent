#include "Prepare.h"
using namespace panda::unievent;

void Prepare::uvx_on_prepare (uv_prepare_t* handle) {
    Prepare* h = hcast<Prepare*>(handle);
    h->call_on_prepare();
}

void Prepare::on_prepare () {
    if (prepare_event.has_listeners()) prepare_event(this);
    else throw ImplRequiredError("Prepare::on_prepare");
}

void Prepare::start (prepare_fn callback) {
    if (callback) prepare_event.add(callback);
    uv_prepare_start(&uvh, uvx_on_prepare);
}

void Prepare::stop () {
    uv_prepare_stop(&uvh);
}

void Prepare::reset () {
    uv_prepare_stop(&uvh);
}

PrepareSP Prepare::call_soon(function<void ()> f, Loop* loop) {
    // capturing object to lambda makes a circle reference. It breaks
    PrepareSP handle = new Prepare(loop);
    handle->prepare_event.add([=](Prepare*) mutable {
        f();
        handle.reset();
    });
    handle->start();
    return handle;
}
