#pragma once
#include "inc.h"

namespace panda { namespace unievent { namespace backend { namespace uv {

template <class BASE, class UVTYPE>
struct UVRequest : BASE {
    bool   active;
    UVTYPE uvr;

    UVRequest (HandleImpl* h, IRequestListener* l) : BASE(h, l), active() {
        uvr.data = static_cast<RequestImpl*>(this);
    }

    void destroy () noexcept override {
        if (active) set_stub(&uvr.cb); // cant make uv request stop, so remove as it completes
        else delete this;
    }

private:
    template <class...Args>
    static inline void set_stub (void (**cbptr)(UVTYPE*, Args...)) {
        *cbptr = [](UVTYPE* p, Args...) { delete get_request(p); };
    }
};

}}}}
