#include "lib/test.h"

TEST_CASE("async", "[async]") {
    AsyncTest test(100, {"async"});

    Async async([&](Async*) {
        test.happens("async");
        test.loop->stop();
    }, test.loop);

    SECTION("from this thread") {
        SECTION("after run") {
            test.loop->call_soon([&]{
                async.send();
            });
        }
        SECTION("before run") {
            async.send();
        }
        test.run();
    }

    SECTION("from another thread") {
        std::thread t;
        SECTION("after run") {
            t = std::thread([](Async* h) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                h->send();
            }, &async);
            test.run();
        }
        SECTION("before run") {
            t = std::thread([](Async* h) {
                h->send();
            }, &async);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            test.run();
        }
        t.join();
    }
}
