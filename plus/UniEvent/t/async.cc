#include "lib/test.h"

TEST_CASE("async", "[async]") {
    SECTION("send") {
        AsyncTest test(100, {"async"});

        AsyncSP async = new Async([&](auto) {
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
                t = std::thread([](auto h) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    h->send();
                }, async);
                test.run();
            }
            SECTION("before run") {
                t = std::thread([](auto h) {
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

        SECTION("event listener") {
            auto s = [&](auto lst) {
                async->event_listener(&lst);
                async->event.add([&](auto){ lst.cnt += 10; });
                async->send();
                test.run_nowait();
                CHECK(lst.cnt == 11);
            };

            SECTION("std") {
                struct Lst : IAsyncListener {
                    int cnt = 0;
                    void on_async (const AsyncSP&) override { ++cnt; }
                };
                s(Lst());
            }
            SECTION("self") {
                struct Lst : IAsyncSelfListener {
                    int cnt = 0;
                    void on_async () override { ++cnt; }
                };
                s(Lst());
            }
        }
    }
}
