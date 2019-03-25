#include "Check.h"
using namespace panda::unievent;

const HandleType Check::TYPE("check");

const HandleType& Check::type () const {
    return TYPE;
}

void Check::start (check_fn callback) {
    if (callback) check_event.add(callback);
    impl()->start();
}

void Check::stop () {
    impl()->stop();
}

void Check::reset () {
    impl()->stop();
}

void Check::on_check () {
    check_event(this);
}

void Check::handle_check () {
    on_check();
}
