#include "Poll.h"
using namespace panda::unievent;

const HandleType Poll::TYPE("poll");

const HandleType& Poll::type () const {
    return TYPE;
}

void Poll::on_poll (int events, const CodeError* err) {
    if (poll_event.has_listeners()) poll_event(this, events, err);
    else throw ImplRequiredError("Poll::on_poll");
}

void Poll::start (int events, poll_fn callback) {
    if (callback) poll_event.add(callback);
    impl()->start(events);
}

void Poll::stop () {
    impl()->stop();
}

void Poll::reset () { stop(); }
