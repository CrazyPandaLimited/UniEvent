#include "FsPoll.h"
using namespace panda::unievent;

const HandleType FsPoll::TYPE("fs_poll");

FsPoll::FsPoll (const LoopSP& loop) : prev(), fetched() {
    _init(loop);
    fsr   = new Fs::Request(loop);
    timer = new Timer(loop);
    timer->event.add([this](auto) {
        if (fsr->busy()) return; // filesystem has not yet completed the request -> skip one cycle
        do_stat();
    });
}

const HandleType& FsPoll::type () const {
    return TYPE;
}

void FsPoll::start (std::string_view path, unsigned int interval, const fs_poll_fn& callback) {
    if (timer->active()) throw Error("cannot start FsPoll: it is already active");
    if (callback) event.add(callback);
    _path = string(path);
    timer->start(interval);
    if (fsr->busy()) fsr = new Fs::Request(loop()); // previous fspoll task has not yet been completed -> forget FSR and create new one
    do_stat();
}

void FsPoll::stop () {
    if (!timer->active()) return;
    timer->stop();
    prev = Fs::Stat();
    // if cancellation possible it will call callback (which we will ignore)
    // otherwise nothing will happen and fsr will remain busy (and if it is not complete by the next start(), we will change fsr
    fsr->cancel();
    fetched = false;
}

void FsPoll::do_stat () {
    if (!wself) wself = FsPollSP(this);
    auto wp = wself;
    fsr->stat(_path, [this, wp](auto& stat, auto& err, const Fs::RequestSP& req) {
        auto p = wp.lock();
        if (!p) return; // check if <this> is dead by the moment, after this line we can safely use 'this'
        if (!timer->active() || fsr != req) return; // ongoing previous result -> ignore

        if (err) {
            if (err != prev_err) {
                prev_err = err;
                on_fs_poll(prev, stat, err); // accessing <stat> is UB
            }
        }
        else if (!fetched || prev != stat) {
            if (fetched) on_fs_poll(prev, stat, err);
            prev = stat;
        }

        fetched = true;
    });
}

void FsPoll::reset () {
    stop();
}

void FsPoll::clear () {
    stop();
    weak(false);
    event.remove_all();
}

void FsPoll::on_fs_poll (const Fs::Stat& prev, const Fs::Stat& cur, const CodeError& err) {
    event(this, prev, cur, err);
}
