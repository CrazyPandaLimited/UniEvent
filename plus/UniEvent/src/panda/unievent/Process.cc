#include "Process.h"
using namespace panda::unievent;

void Process::uvx_on_exit (uv_process_t* handle, int64_t exit_status, int term_signal) {
    Process* h = hcast<Process*>(handle);
    h->call_on_exit(exit_status, term_signal);
}

void Process::spawn (ProcessOptions* options, exit_fn callback) {
    if (callback) exit_event.add(callback);
    uv_process_options_t* uvopts = _pex_(options);
    uvopts->exit_cb = uvx_on_exit;
    int err = uv_spawn(uvh.loop, &uvh, uvopts);
    if (err) throw CodeError(err);
}

void Process::kill (int signum) {
    int err = uv_process_kill(&uvh, signum);
    if (err) throw CodeError(err);
}

void Process::reset () {
    kill(SIGINT);
}

void Process::kill (int pid, int signum) {
    int err = uv_kill(pid, signum);
    if (err) throw CodeError(err);
}

void Process::get_title (char* buffer, size_t size) {
    int err = uv_get_process_title(buffer, size);
    if (err) throw CodeError(err);
}

void Process::set_title (const char* title) {
    int err = uv_set_process_title(title);
    if (err) throw CodeError(err);
}

void Process::on_exit (int64_t exit_status, int term_signal) {
    if (exit_event.has_listeners()) exit_event(this, exit_status, term_signal);
    throw ImplRequiredError("Process::on_exit");
}
