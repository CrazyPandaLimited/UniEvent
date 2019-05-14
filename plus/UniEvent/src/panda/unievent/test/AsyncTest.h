#pragma once
#include <panda/unievent.h>
#include <panda/net/sockaddr.h>
#include <panda/CallbackDispatcher.h>

namespace panda { namespace unievent { namespace test {

template <typename T>
using sp = iptr<T>;

using panda::string;

struct AsyncTest {
    using SockAddr = panda::net::SockAddr;
    LoopSP loop;
    std::vector<string> expected;
    std::vector<string> happened;
    TimerSP timer;

    struct Error : std::runtime_error {
        Error(std::string msg, AsyncTest& test);
    };

    static SockAddr get_refused_addr   ();
    static SockAddr get_blackhole_addr ();

    AsyncTest (uint64_t timeout, unsigned count = 0, const LoopSP& loop = nullptr);
    AsyncTest (uint64_t timeout, const std::vector<string>& expected, const LoopSP& loop = nullptr);
    virtual ~AsyncTest() noexcept(false);

    void set_expected (unsigned);
    void set_expected (const std::vector<string>&);

    void run        ();
    void run_once   ();
    void run_nowait ();
    void happens    (string event = "<event>");

    template<typename F>
    static TimerSP timer_once (uint64_t timeout, Loop* loop, F&& f) {
        TimerSP timer = new Timer(loop);
        timer->once(timeout);
        timer->event.add([f](Timer* t) {
            t->stop();
            f();
        });
        return timer;
    }

    template<typename F>
    TimerSP timer_once (uint64_t timeout, F&& f) {
        return timer_once(timeout, loop, std::forward<F>(f));
    }

    template <class T> static inline T _await_copy (T arg) { return arg; }
    static inline CodeError _await_copy (const CodeError& err) { return err; }

    template <typename Ret, typename... Args, typename Dispatcher = CallbackDispatcher<Ret(Args...)>>
    std::tuple<decltype(_await_copy(std::declval<Args>()))...>
    await (CallbackDispatcher<Ret(Args...)>& dispatcher, string event = "") {
        using Callback = typename Dispatcher::Callback;
        std::tuple<decltype(_await_copy(std::declval<Args>()))...> result;
        Callback wrapper = [&](typename Dispatcher::Event& e, Args... args) {
            loop->stop();
            e.dispatcher.remove(wrapper);
            result = std::make_tuple(_await_copy(args)...);
            happens(event);
            e.next(args...);
        };
        dispatcher.add(wrapper);

        run();
        return result;
    }

    template <typename... Args>
    std::tuple<decltype(_await_copy(std::declval<Args>()))...>
    await (panda::function<void(Args...)>& cb, string event = "") {
        using Function = panda::function<void(Args...)>;
        std::tuple<decltype(_await_copy(std::declval<Args>()))...> result;
        Function prev = cb;
        Function wrapper = [&](Args... args) -> void {
            loop->stop();
            result = std::make_tuple(_await_copy(args)...);
            happens(event);
            if (prev) {
                return prev(args...);
            } else {
                return;
            }
        };
        cb = wrapper;
        run();
        cb = prev;
        return result;
    }

    template <typename Ret, typename... Args>
    void await (const std::vector<CallbackDispatcher<Ret(Args...)>*>& v, string event = "") {
        size_t cnt = v.size();

        auto action = [&]() {
            if (--cnt) return;
            loop->stop();
            happens(event);
        };

        for (auto d : v) wrap_dispatcher(*d, action);

        run();
    }

    template <typename... Dispatchers>
    void await_multi (Dispatchers&... dispatchers) {
        size_t counter = sizeof...(dispatchers);
        auto action = [&]() {
            if (--counter == 0) {
                loop->stop();
            }
        };

        auto fake = {wrap_dispatcher(dispatchers, action)...}; (void)fake;
        run();
    }

    /** return true if callback was not called **/
    template <typename T>
    bool await_not (T&& f, uint64_t timeout) {
        bool by_timer = false;
        TimerSP timer = Timer::once(timeout, [&](Timer*) {
            by_timer = true;
            loop->stop();
        }, loop); (void)timer;

        await(f);
        return by_timer;
    }

    bool wait(uint64_t timeout) {
        bool by_timer = false;
        TimerSP timer = Timer::once(timeout, [&](Timer*) {
            by_timer = true;
            loop->stop();
        }, loop); (void)timer;
        run();
        return by_timer;
    }

protected:
    virtual std::string generate_report ();
    string destroy_loop (); // return error if smth foes wrong

private:
    template <typename Action, typename Ret, typename... Args, typename Dispatcher = CallbackDispatcher<Ret(Args...)>>
    int wrap_dispatcher (CallbackDispatcher<Ret(Args...)>& dispatcher, Action& action) {
        typename Dispatcher::Callback wrapper(
            [&](Ifunction<Ret, typename Dispatcher::Event&, Args...>& self, typename Dispatcher::Event& e, Args... args) {
                e.dispatcher.remove(self);
                action();
                e.next(args...);
            }
        );
        dispatcher.add(wrapper);
        return 1; // for fake foreach
    }

    sp<Timer> create_timeout (uint64_t timeout);
    bool happened_as_expected ();
};

}}}
