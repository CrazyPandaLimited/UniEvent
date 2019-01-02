#include "Prepare.h"
using namespace panda::unievent;

const HandleType Prepare::TYPE("prepare");

const HandleType& Prepare::type () const {
    return TYPE;
}

void Prepare::start (prepare_fn callback) {
    if (callback) prepare_event.add(callback);
    impl()->start();
}

void Prepare::stop () {
    impl()->stop();
}

void Prepare::reset () {
    stop();
}

void Prepare::on_prepare () {
    if (prepare_event.has_listeners()) prepare_event(this);
    else throw ImplRequiredError("Prepare::on_prepare");
}

PrepareSP Prepare::call_soon (function<void ()> f, Loop* loop) {
    // capturing object to lambda makes a circle reference. It breaks
    PrepareSP handle = new Prepare(loop);
    handle->prepare_event.add([=](Prepare*) mutable {
        f();
        handle.reset();
    });
    handle->start();
    return handle;
}
