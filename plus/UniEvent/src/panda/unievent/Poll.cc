#include "Poll.h"
using namespace panda::unievent;

const HandleType Poll::TYPE("poll");

const HandleType& Poll::type () const {
    return TYPE;
}

void Poll::start (int events, poll_fn callback) {
    if (callback) event.add(callback);
    impl()->start(events);
}

void Poll::stop () {
    impl()->stop();
}

void Poll::reset () {
    stop();
}

void Poll::clear () {
    stop();
    event.remove_all();
}

void Poll::on_poll (int events, const CodeError& err) {
    event(this, events, err);
}

void Poll::handle_poll (int events, const CodeError& err) {
    on_poll(events, err);
}
