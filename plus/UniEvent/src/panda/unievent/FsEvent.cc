#include "FsEvent.h"
using namespace panda::unievent;

const HandleType FsEvent::TYPE("fs_event");

const HandleType& FsEvent::type () const {
    return TYPE;
}

void FsEvent::start (const std::string_view& path, int flags, fs_event_fn callback) {
    if (callback) event.add(callback);
    _path = string(path);
    impl()->start(path, flags);
}

void FsEvent::stop () {
    impl()->stop();
}

void FsEvent::reset () {
    impl()->stop();
}

void FsEvent::clear () {
    impl()->stop();
    event.remove_all();
    weak(false);
}

void FsEvent::handle_fs_event (const std::string_view& file, int events, const CodeError& err) {
    on_fs_event(file, events, err);
}

void FsEvent::on_fs_event (const std::string_view& file, int events, const CodeError& err) {
    event(this, file, events, err);
}
