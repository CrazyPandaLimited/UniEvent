#include "lib/test.h"

TEST_CASE("async simple", "[async_test]") {
    bool called = false;
    AsyncTest test(200, {"timer"});

    auto timer = test.timer_once(10, [&]() {
        called = true;
    });
    auto res = test.await(timer->timer_event, "timer");
    REQUIRE(called);
    REQUIRE(std::get<0>(res) == timer.get());
}

TEST_CASE("async dispatcher", "[async_test]") {
    bool called = false;
    AsyncTest test(200, {"dispatched"});

    CallbackDispatcher<void(int)> d;
    auto timer1 = test.timer_once(10, [&]() {
        called = true;
        d(10);
    });

    auto res = test.await(d, "dispatched");
    REQUIRE(called);
    REQUIRE(std::get<0>(res) == 10);
}


TEST_CASE("async multi", "[async_test]") {
    int called = 0;
    AsyncTest test(200, {});

    CallbackDispatcher<void(void)> d1;
    auto timer1 = test.timer_once(10, [&]() {
        called++;
        d1();
    });
    CallbackDispatcher<void(void)> d2;
    auto timer2 = test.timer_once(20, [&]() {
        called++;
        d2();
    });

    test.await_multi(d2, d1);
    REQUIRE(called == 2);
}

TEST_CASE("call_soon", "[async_test]") {
    AsyncTest test(200, {"call"});
    size_t count = 0;
    test.loop->call_soon([&]() {
        count++;
        if (count >= 2) FAIL("called twice");
        test.happens("call");
        test.loop->stop();
    });
    test.run();
    TimerSP timer = Timer::once(50, [&](Timer*){
        test.loop->stop();
    }, test.loop);
    REQUIRE(count == 1);
}
