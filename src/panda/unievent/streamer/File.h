#pragma once
#include "../Fs.h"
#include "../Streamer.h"

namespace panda { namespace unievent { namespace streamer {

struct FileInput : Streamer::IInput {
    FileInput (string_view path, size_t chunk_size = 1000000) : path(string(path)), chunk_size(chunk_size) {}

    ErrorCode start (const LoopSP&) override;
    void      stop  () override;

    ErrorCode start_reading () override;
    void      stop_reading  () override;

private:
    string        path;
    size_t        chunk_size;
    LoopSP        loop;
    fd_t          fd;
    Fs::RequestSP fsreq;
    bool          opened = false;
    bool          pause = false;

    void do_read ();
    void on_read (const string&, const std::error_code&, const Fs::RequestSP&);
};

}}}
