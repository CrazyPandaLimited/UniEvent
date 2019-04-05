#pragma once
#include "inc.h"

namespace panda { namespace unievent { namespace backend { namespace uv {

template <class Base, class UvReq>
struct UVRequest : Base {
    bool  active;
    UvReq uvr;

    template <class...Args>
    UVRequest (Args&&...args) : Base(args...), active() {
        uvr.data = static_cast<BackendRequest*>(this);
    }

    BackendHandle* handle () const noexcept override {
        return get_handle(uvr.handle);
    }

    void destroy () noexcept override {
        if (active) set_stub(&uvr.cb); // cant make uv request stop, so remove as it completes
        else delete this;
    }

private:
    template <class...Args>
    static inline void set_stub (void (**cbptr)(UvReq*, Args...)) {
        *cbptr = [](UvReq* p, Args...) { delete get_request(p); };
    }
};

}}}}
