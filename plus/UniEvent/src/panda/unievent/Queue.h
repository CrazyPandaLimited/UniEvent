#pragma once
#include "Request.h"
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

struct Queue {
    Queue () : recurse() {}

    void push (const RequestSP& req) {
        requests.push_back(req);
        if (requests.size() == 1 && !recurse) req->exec();
    }

    RequestSP done (Request* check) {
        auto ret = std::move(requests.front());
        assert(ret == check);
        requests.pop_front();
        return ret;
    }

    void next () {
        requests.pop_front();
        resume();
    }

    void abort () {
        if (!requests.size()) return;
        ++recurse;
        recurse_last = requests.back();

        scope_guard([&] {
            RequestSP cur;
            do {
                cur = requests.front();
                requests.pop_front();
                cur->abort();
            } while (recurse_last && cur != recurse_last);
        }, [&] {
            recurse_last = nullptr;
            --recurse;
        });
    }

    void resume () {
        if (requests.size() && !recurse) requests.front()->exec();
    }

    bool busy () const { return requests.size(); }

private:
    using Requests = panda::lib::IntrusiveChain<RequestSP>;
    Requests  requests;
    uint32_t  recurse;
    RequestSP recurse_last;
};

}}
