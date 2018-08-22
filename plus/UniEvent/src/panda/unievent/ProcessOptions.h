#pragma once
#include <cstring>
#include <panda/unievent/inc.h>
#include <panda/unievent/StdioContainer.h>

namespace panda { namespace unievent {

class Process;

class ProcessOptions {
public:
    enum process_flags {
      PROCESS_SETUID = UV_PROCESS_SETUID,
      PROCESS_SETGID = UV_PROCESS_SETGID,
      PROCESS_WINDOWS_VERBATIM_ARGUMENTS = UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS,
      PROCESS_DETACHED = UV_PROCESS_DETACHED,
      PROCESS_WINDOWS_HIDE = UV_PROCESS_WINDOWS_HIDE
    };

    ProcessOptions () {
        std::memset(&opts, 0, sizeof(opts));
    }

    const char*     file    () { return opts.file; }
    char**          args    () { return opts.args; }
    char**          env     () { return opts.env; }
    const char*     cwd     () { return opts.cwd; }
    unsigned int    flags   () { return opts.flags; }
    uid_t           uid     () { return opts.uid; }
    gid_t           gid     () { return opts.gid; }
    StdioContainer* stdio   () { return reinterpret_cast<StdioContainer*>(opts.stdio); }

    void file    (const char* val)  { opts.file = val; }
    void args    (char** val)       { opts.args = val; }
    void env     (char** val)       { opts.env = val; }
    void cwd     (char* val)        { opts.cwd = val; }
    void flags   (unsigned int val) { opts.flags = val; }
    void uid     (uid_t val)        { opts.uid = val; }
    void gid     (gid_t val)        { opts.gid = val; }

    void stdio (StdioContainer* stdios, int count) {
        opts.stdio = reinterpret_cast<uv_stdio_container_t*>(stdios);
        opts.stdio_count = count;
    }

    friend uv_process_options_t* _pex_ (ProcessOptions*);

private:
    uv_process_options_t opts;
};

inline uv_process_options_t* _pex_ (ProcessOptions* popts) { return &popts->opts; }

}}
