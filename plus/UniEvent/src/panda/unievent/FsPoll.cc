#include "FsPoll.h"
using namespace panda::unievent;

const HandleType FsPoll::TYPE("fs_poll");

const HandleType& FsPoll::type () const {
    return TYPE;
}

void FsPoll::start (std::string_view path, unsigned int interval, const fs_poll_fn& callback) {
    if (callback) event.add(callback);
    impl()->start(path, interval);
}

void FsPoll::stop () {
    impl()->stop();
}

void FsPoll::reset () {
    impl()->stop();
}

void FsPoll::clear () {
    impl()->stop();
    event.remove_all();
}

panda::string FsPoll::path () const {
    return impl()->path();
}

void FsPoll::on_fs_poll (const Stat& prev, const Stat& cur, const CodeError& err) {
    event(this, prev, cur, err);
}

void FsPoll::handle_fs_poll (const Stat& prev, const Stat& cur, const CodeError& err) {
    on_fs_poll(prev, cur, err);
}
