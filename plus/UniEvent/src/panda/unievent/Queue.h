#pragma once
#include "Request.h"
#include <panda/lib/intrusive_chain.h>

namespace panda { namespace unievent {

struct ExceptionKeeper {
    template <class F>
    void etry (F&& f) {
        try { f(); }
        catch (...) {
            auto e = std::current_exception();
            assert(e);
            exceptions.push_back(e);
        }
    }

    void throw_if_any () {
        if (exceptions.size()) {
            std::rethrow_exception(exceptions[0]); // TODO: throw all exceptions as nested
        }
    }

private:
    std::vector<std::exception_ptr> exceptions;
};


struct Queue {
    Queue () : locked(), cancel_gen() {}

    void push (const RequestSP& req) {
        requests.push_back(req);
        if (requests.size() == 1 && !locked) req->exec();
    }

    template <class Func>
    void done (Request* check, Func&& f) {
        _EDEBUG("req=%p", check);
        auto req = requests.front();
        assert(req == check);
        if (req == cancel_till) cancel_till = nullptr; // stop canceling, as we processed the last request
        req->finish_exec();
        requests.pop_front();
        ++locked; // lock, so if f() add anything it won't get executed
        scope_guard([&] {
            f();
        }, [&] {
            --locked;
            resume(); // execute what has been added
        });
    }

    template <class Post>
    void cancel (Post&& f) { cancel([]{}, f); }

    template <class Pre, class Post>
    void cancel (Pre&& fpre, Post&& fpost) {
        ++locked; // this blocks executing of requests
        cancel_till = requests.back(); // we must not cancel anything that is added during callbacks execution
        _EDEBUG("cancel_till=%p size=%lu", cancel_till.get(), requests.size());
        auto gen = cancel_gen;

        ExceptionKeeper exk;

        _EDEBUG("run fpre");
        exk.etry([&]{ fpre(); });

        _EDEBUG("2 cancel_till=%p size=%lu", cancel_till.get(), requests.size());

        if (requests.size() && cancel_till) {
            RequestSP cur;
            do {
                cur = requests.front();
                _EDEBUG("removing next request %p", cur.get());
                exk.etry([&]{ cur->cancel(); });
                assert(cur != requests.front()); // if cancel() throws before calling done(), otherwise infite loop. Idea to prettify?
            } while (cancel_till && cur != cancel_till); // respect recursive cancel()
        }

        // post callback is only called if no recursive cancels has been made
        _EDEBUG("run fpost");
        if (gen == cancel_gen) exk.etry([&]{
            fpost();
            ++cancel_gen;
        });

        cancel_till = nullptr;
        --locked;

        resume(); // execute what has been added

        exk.throw_if_any();
    }

    void resume () {
        _EDEBUG();
        auto& req = requests.front();
        if (req && !locked) req->exec();
    }

    size_t size      () const { return requests.size(); }
    bool   canceling () const { return cancel_till; }

private:
    using Requests = panda::lib::IntrusiveChain<RequestSP>;
    Requests  requests;
    uint32_t  locked;
    uint32_t  cancel_gen;
    RequestSP cancel_till;
};

}}
