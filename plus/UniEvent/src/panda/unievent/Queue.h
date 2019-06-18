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
        req->async = false;
        requests.push_back(req);
        if (requests.size() == 1 && !locked) req->exec();
        req->async = true;
    }

    template <class Func>
    void done (Request* check, Func&& f) {
        auto req = requests.front();
        assert(req == check);
        if (req == cancel_till) cancel_till = nullptr; // stop canceling, as we processed the last request
        req->finish_exec();
        requests.pop_front();

        if (req->async) { // we can call user callback right now
            ++locked; // lock, so if f() add anything it won't get executed
            scope_guard([&] {
                f();
            }, [&] {
                --locked;
                resume(); // execute what has been added
            });
        } else { // we must delay user callback
            finalized_requests.push_back(req);
            req->delay([=]{
                assert(finalized_requests.front() == req);
                finalized_requests.pop_front();
                f();
            });
        }
    }

    template <class Post>
    void cancel (Post&& f) { cancel([]{}, f); }

    template <class Pre, class Post>
    void cancel (Pre&& fpre, Post&& fpost) {
        ++locked; // this blocks executing of requests
        cancel_till = requests.back(); // we must not cancel anything that is added during callbacks execution
        auto gen = cancel_gen;

        ExceptionKeeper exk;

        while (finalized_requests.size()) { // don't need <till> because can't be added during callbacks because <locked>
            auto cur = finalized_requests.front();
            finalized_requests.pop_front();
            cur->delay_cancel();
            exk.etry([&]{ cur->notify(CodeError(std::errc::operation_canceled)); });
        }

        exk.etry([&]{ fpre(); });

        if (requests.size() && cancel_till) {
            RequestSP cur;
            do {
                cur = requests.front();
                exk.etry([&]{ cur->cancel(); });
                assert(cur != requests.front()); // if cancel() throws before calling done(), otherwise infite loop. Idea to prettify?
            } while (cancel_till && cur != cancel_till); // respect recursive cancel()
        }

        // post callback is only called if no recursive cancels has been made
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
        auto& req = requests.front();
        if (req && !locked) req->exec();
    }

    size_t size      () const { return requests.size(); }
    bool   canceling () const { return cancel_till; }

private:
    using Requests = panda::lib::IntrusiveChain<RequestSP>;
    Requests  requests;
    Requests  finalized_requests;
    uint32_t  locked;
    uint32_t  cancel_gen;
    RequestSP cancel_till;
};

}}
