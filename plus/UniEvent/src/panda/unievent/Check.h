#pragma once
#include <panda/unievent/Handle.h>

namespace panda { namespace unievent {

class Check : public virtual Handle {
public:
    using check_fptr = void(Check*);
    using check_fn = panda::function<check_fptr>;

    CallbackDispatcher<check_fptr> check_event;

    Check (Loop* loop = Loop::default_loop()) {
        int err = uv_check_init(_pex_(loop), &uvh);
        if (err) throw CheckError(err);
        _init(&uvh);
    }

    virtual void start (check_fn callback = nullptr);
    virtual void stop  ();

    void reset () override;

    void call_on_check () { on_check(); }

protected:
    virtual void on_check ();

private:
    uv_check_t uvh;

    static void uvx_on_check (uv_check_t* handle);
};

}}
