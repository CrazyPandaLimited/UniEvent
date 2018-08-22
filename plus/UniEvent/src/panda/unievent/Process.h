#pragma once
#include <panda/unievent/Handle.h>
#include <panda/unievent/ProcessOptions.h>

namespace panda { namespace unievent {

class Process : public virtual Handle {
    using exit_fptr = void(Process* handle, int64_t exit_status, int term_signal);
    using exit_fn = function<exit_fptr>;
    
    CallbackDispatcher<exit_fptr> exit_event;

    Process (Loop* loop = Loop::default_loop()) {
        uvh.loop = _pex_(loop);
        _init(&uvh);
    }

    virtual void spawn (ProcessOptions* options, exit_fn callback = nullptr);
    virtual void kill  (int signum);

    void reset () override;

    static void kill      (int pid, int signum);
    static void get_title (char* buffer, size_t size);
    static void set_title (const char* title);

    void call_on_exit (int64_t exit_status, int term_signal) { on_exit(exit_status, term_signal); }

protected:
    virtual void on_exit (int64_t exit_status, int term_signal);

private:
    uv_process_t uvh;

    static void uvx_on_exit (uv_process_t* handle, int64_t exit_status, int term_signal);
};

}}
