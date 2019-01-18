#include "Async.h"
using namespace panda::unievent;

const HandleType Async::TYPE("async");

const HandleType& Async::type () const {
    return TYPE;
}

void Async::send () {
    impl()->send();
}

void Async::reset () {}

void Async::on_async () {
    if (async_event.has_listeners()) async_event(this);
    else throw ImplRequiredError("Async::on_async");
}
