#include "Async.h"
using namespace panda::unievent;

const HandleType Async::TYPE("async");

const HandleType& Async::type () const {
    return TYPE;
}

void Async::send () {
    impl()->send();
}

void Async::on_async () {
    event(this);
}

void Async::handle_async () {
    on_async();
}
