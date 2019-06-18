#include <catch.hpp>
#include <chrono>
#include <panda/unievent/test/AsyncTest.h>
#include <panda/unievent/Timer.h>

using namespace panda;
using namespace unievent;
using namespace test;

static int64_t get_time() {
    using namespace std::chrono;
    return duration_cast< milliseconds >(steady_clock::now().time_since_epoch()).count();
}

#define REQUIRE_ELAPSED(T0, EXPECTED) do{auto diff = get_time() - T0; REQUIRE(diff >= EXPECTED); REQUIRE((diff / EXPECTED) < 1.6);} while(0)

TEST_CASE("Timer static once", "[timer]") {
    auto t0 = get_time();
    AsyncTest test(200, {"timer"});
    int timeout = 30;
    auto timer = Timer::once(timeout, [&](Timer*) {
        test.happens("timer");
        REQUIRE_ELAPSED(t0, timeout);
    }, test.loop);
    test.await(timer->event);
}

TEST_CASE("Timer static repeat", "[timer]") {
    AsyncTest test(200, {"timer", "timer", "timer"});
    int timeout = 30;
    auto t0 = get_time();
    size_t counter = 3;
    auto timer = Timer::start(timeout, [&](Timer* t) {
        test.happens("timer");
        REQUIRE_ELAPSED(t0, timeout);
        if (--counter == 0) {
            t->stop();
            test.loop->stop();
        }
        t0 = get_time();
    }, test.loop);
    test.run();
}
