#include "lib/test.h"

TEST_CASE("async", "[async]") {
    SECTION("send") {
        AsyncTest test(100, {"async"});

        AsyncSP async = new Async([&](Async*) {
            test.happens("async");
            test.loop->stop();
        }, test.loop);

        SECTION("from this thread") {
            SECTION("after run") {
                test.loop->delay([&]{
                    async->send();
                });
            }
            SECTION("before run") {
                async->send();
            }
            test.run();
        }

        SECTION("from another thread") {
            std::thread t;
            SECTION("after run") {
                t = std::thread([](Async* h) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    h->send();
                }, async);
                test.run();
            }
            SECTION("before run") {
                t = std::thread([](Async* h) {
                    h->send();
                }, async);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                test.run();
            }
            t.join();
        }

        SECTION("call_now") {
            async->call_now();
        }
    }

    SECTION("zombie mode") {
        LoopSP loop = new Loop();
        AsyncSP async = new Async([](Async*){
            FAIL("should not be called");
        }, loop);
        loop.reset();

        REQUIRE_THROWS( async->send() );
    }
}
