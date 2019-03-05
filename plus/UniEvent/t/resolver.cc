#include "lib/test.h"
#include <thread>
#include <sstream>
#include <set>

TEST_CASE("resolver", "[resolver]") {
    AsyncTest test(2000, {});
    ResolverSP resolver = new Resolver(test.loop);
    std::vector<AddrInfo> res;
    auto success_cb = [&](const Resolver::RequestSP&, const AddrInfo& ai, const CodeError* err) {
        test.happens("r");
        CHECK(!err);
        CHECK(ai);
        res.push_back(ai);
    };
    auto req = resolver->resolve()->node("localhost")->on_resolve(success_cb);
    int expected_cnt = 2;

    SECTION("no cache") {
        req->use_cache(false);

        req->run();
        test.run();
        CHECK(resolver->cache_size() == 0);

        req->run();
        test.run();
        CHECK(resolver->cache_size() == 0);
        CHECK(!res[0].is(res[1])); // without cache every resolve is executed
        CHECK(res[0] == res[1]); // but result is the same
    }

    SECTION("cache") {
        SECTION("no hints") {
            // Resolver will use cache by default, first time it is not in cache, async call
            req->run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            // in cache, so the call is sync
            req->run();
            test.run_nowait();
            CHECK(resolver->cache_size() == 1);
            CHECK(res[0].is(res[1]));
        }

        SECTION("both empty hints") {
            req->hints(AddrInfoHints());

            req->run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            req->run();
            test.run_nowait();
            CHECK(resolver->cache_size() == 1);
            CHECK(res[0].is(res[1]));
        }

        SECTION("custom hints and empty hints") {
            req->hints(AddrInfoHints(AF_INET));

            req->run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            req->hints(AddrInfoHints());
            req->run();
            test.run();
            CHECK(resolver->cache_size() == 2);
            CHECK(!res[0].is(res[1]));
        }

        SECTION("different hints") {
            req->hints(AddrInfoHints(AF_INET));

            req->run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            req->hints(AddrInfoHints(AF_INET6));
            req->run();
            test.run();
            CHECK(resolver->cache_size() == 2);
            CHECK(!res[0].is(res[1]));
        }
    }

    SECTION("service/port") {
        req->service("80");

        req->run();
        test.run();
        CHECK(resolver->cache_size() == 1);

        req->service("");
        req->port(80);
        req->run();
        test.run();
        CHECK(resolver->cache_size() == 1);
        CHECK(res[0].is(res[1]));

        for (auto ai = res[0]; ai; ai = ai.next()) {
            auto addr = ai.addr();
            CHECK(addr.port() == 80);
            if      (addr.is_inet4()) CHECK(addr.ip() == "127.0.0.1");
            else if (addr.is_inet6()) CHECK(addr.ip() == "::1");
        }
    }

    SECTION("cache limit") {
        expected_cnt = 3;
        resolver = new Resolver(test.loop, 500, 2);
        auto req = resolver->resolve()->node("localhost")->on_resolve(success_cb);

        req->port(80);
        req->run();
        test.run();
        CHECK(resolver->cache_size() == 1);

        req->port(443);
        req->run();
        test.run();
        CHECK(resolver->cache_size() == 2);

        req->port(22);
        req->run();
        test.run();
        CHECK(resolver->cache_size() == 1);
    }

    SECTION("timeout") {
        // will make it in required time
        resolver->resolve("localhost", [&](Resolver::RequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            CHECK(!err);
            CHECK(ai);
        }, 1000);
        test.run();
        CHECK(resolver->cache_size() == 1);

        // will not make it
        resolver->resolve("ya.ru", [&](Resolver::RequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            REQUIRE(err);
            CHECK(err->code() == std::errc::timed_out);
            CHECK(!ai);
        }, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        test.run();
        CHECK(resolver->cache_size() == 1);
    }

    auto canceled_cb = [&](Resolver::RequestSP, const AddrInfo& ai, const CodeError* err) {
        test.happens("r");
        REQUIRE(err);
        CHECK(err->code() == std::errc::operation_canceled);
        CHECK(!ai);
    };

    SECTION("cancel") {
        expected_cnt = 1;

        Resolver::RequestSP req;

        SECTION("not cached") {
            SECTION("ares-async") {
                req = resolver->resolve("tut.by", canceled_cb);
                SECTION("sync") {
                    req->cancel();
                }
                SECTION("async") {
                    test.loop->delay([=]{
                        req->cancel();
                    });
                }
            }
            SECTION("ares-sync") {
                SECTION("sync") {
                    req = resolver->resolve("localhost", canceled_cb);
                    req->cancel();
                }
                SECTION("async") {
                    test.loop->delay([=]{
                        req->cancel();
                    });
                    req = resolver->resolve("localhost", canceled_cb);
                }
            }
        }
        test.run();

        req->cancel(); // should be no-op
    }

    SECTION("reset") {
        expected_cnt = 3;
        auto req = resolver->resolve("lenta.ru", canceled_cb);
        resolver->resolve("mail.ru", canceled_cb);

        SECTION("sync") {
            resolver->resolve("localhost", canceled_cb);
            resolver->reset();
        }
        SECTION("async") {
            test.loop->delay([&]{
                resolver->reset();
            });
            resolver->resolve("localhost", canceled_cb); // our delay must be the first
        }

        test.run();

        req->cancel(); // should not die
    }

//    SECTION("resolver destroy") {
//        auto req = resolver->resolve("lenta.ru", cancel_cb);
//        resolver->resolve("mail.ru", cancel_cb);
//
//        SECTION("sync") {
//            expected_cnt = 3;
//            resolver->resolve("localhost", cancel_cb); // in async it may complete earlier than our loop->delay
//            resolver = nullptr;
//        }
//        SECTION("async") {
//            expected_cnt = 2;
//            test.loop->delay([&]{
//                resolver = nullptr;
//            });
//        }
//
//        test.run();
//
//        req->cancel();
//    }

//    SECTION("loop destroy") {
//        LoopSP loop = new Loop();
//        resolver = new Resolver(loop);
//
//        auto req = resolver->resolve("lenta.ru", cancel_cb);
//        resolver->resolve("mail.ru", cancel_cb);
//
//        SECTION("sync") {
//            expected_cnt = 3;
//            resolver->resolve("localhost", cancel_cb); // in async it may complete earlier than our loop->delay
//            loop = nullptr;
//        }
//        SECTION("async") {
//            expected_cnt = 2;
//            test.loop->delay([&]{
//                loop = nullptr;
//            });
//            test.run();
//        }
//
//        CHECK_THROWS(resolver->resolve("localhost", cancel_cb));
//        resolver->reset();
//        req->cancel();
//    }
//
    while (expected_cnt-- > 0) test.expected.push_back("r");

    test.loop->dump();
}
