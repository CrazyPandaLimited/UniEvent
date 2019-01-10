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
