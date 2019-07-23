#include "Check.h"
using namespace panda::unievent;

const HandleType Check::TYPE("check");

const HandleType& Check::type () const {
    return TYPE;
}

void Check::start (check_fn callback) {
    if (callback) event.add(callback);
    impl()->start();
}

void Check::stop () {
    impl()->stop();
}

void Check::reset () {
    impl()->stop();
}

void Check::clear () {
    impl()->stop();
    event.remove_all();
    weak(false);
}

void Check::on_check () {
    event(this);
}

void Check::handle_check () {
    on_check();
}
