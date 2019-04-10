#include "Prepare.h"
using namespace panda::unievent;

const HandleType Prepare::TYPE("prepare");

const HandleType& Prepare::type () const {
    return TYPE;
}

void Prepare::start (prepare_fn callback) {
    if (callback) event.add(callback);
    impl()->start();
}

void Prepare::stop () {
    impl()->stop();
}

void Prepare::reset () {
    impl()->stop();
}

void Prepare::clear () {
    impl()->stop();
    event.remove_all();
}

void Prepare::on_prepare () {
    event(this);
}

void Prepare::handle_prepare () {
    on_prepare();
}
