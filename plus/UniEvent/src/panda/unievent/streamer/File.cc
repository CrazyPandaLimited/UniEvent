#include "File.h"

namespace panda { namespace unievent { namespace streamer {

ErrorCode FileInput::start (const LoopSP& loop) {
    this->loop = loop;
    fsreq = Fs::open(path, Fs::OpenFlags::RDONLY, Fs::DEFAULT_FILE_MODE, [this](fd_t fd, const std::error_code& err, const Fs::RequestSP&) {
        if (err) return handle_read({}, err);
        this->fd = fd;
        opened = true;
        do_read();
    }, loop);
    return {};
}

void FileInput::do_read () {
    Fs::read(fd, chunk_size, -1, [this](const string& data, const std::error_code& err, const Fs::RequestSP& req){
        on_read(data, err, req);
    }, loop);
}

void FileInput::on_read (const string& data, const std::error_code& err, const Fs::RequestSP&) {
    if (err) return handle_read({}, err);
    if (data.length()) handle_read(data, err);
    if (data.length() < chunk_size) return handle_eof();
    if (!pause) do_read();
}

void FileInput::stop () {
    if (fsreq) fsreq->cancel();
    fsreq = nullptr;
    if (opened) Fs::close(fd, [](auto...){}, loop);
    opened = false;
}

ErrorCode FileInput::start_reading () {
    pause = false;
    if (!fsreq->busy()) do_read();
    return {};
}

void FileInput::stop_reading () {
    pause = true;
}

}}}
