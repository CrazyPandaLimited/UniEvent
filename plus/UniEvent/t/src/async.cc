#include <catch.hpp>
#include <panda/unievent/test/AsyncTest.h>

#include "test.h"

using namespace panda::unievent;
using test::AsyncTest;
using test::sp;
using panda::CallbackDispatcher;
using panda::Ifunction;

#ifdef TEST_ASYNC

TEST_CASE("async simple") {
    bool called = false;
    AsyncTest test(200, {"timer"});

    auto timer = test.timer_once(10, [&]() {
        called = true;
    });
    auto res = test.await(timer->timer_event, "timer");
    REQUIRE(called);
    REQUIRE(std::get<0>(res) == timer.get());
}

TEST_CASE("async CallbackDispatcher") {
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


TEST_CASE("async multi") {
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

#endif

