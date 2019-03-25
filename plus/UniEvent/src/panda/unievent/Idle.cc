#include "Idle.h"
using namespace panda::unievent;

const HandleType Idle::TYPE("idle");

const HandleType& Idle::type () const {
    return TYPE;
}

void Idle::start (idle_fn callback) {
    if (callback) idle_event.add(callback);
    impl()->start();
}

void Idle::stop () {
    impl()->stop();
}

void Idle::reset () {
    impl()->stop();
}

void Idle::on_idle () {
    idle_event(this);
}

void Idle::handle_idle () {
    on_idle();
}
