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
    impl()->stop();
}

void Prepare::on_prepare () {
    prepare_event(this);
}

void Prepare::handle_prepare () {
    on_prepare();
}
