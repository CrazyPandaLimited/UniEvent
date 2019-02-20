#include "lib/test.h"
#include <thread>
#include <sstream>
#include <set>

//std::string dump(addrinfo* ai) {
//    std::stringstream ss;
//    for(auto current = ai; current; current = current->ai_next) {
//        ss << SockAddr(ai->ai_addr) << "\n";
//    }
//    return ss.str();
//}

TEST_CASE("resolver", "[resolver]") {
    AsyncTest test(2000, {});
    ResolverSP resolver = new Resolver(test.loop);
    std::vector<AddrInfo> res;
    auto cb = [&](ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
        test.happens("r");
        CHECK(!err);
        CHECK(ai);
        res.push_back(ai);
    };
    auto b = resolver->resolve().node("localhost").on_resolve(cb);
    int expected_cnt = 2;

    SECTION("no cache") {
        b.use_cache(false);

        b.run();
        test.run();
        CHECK(resolver->cache_size() == 0);

        b.run();
        test.run();
        CHECK(resolver->cache_size() == 0);
        CHECK(!res[0].is(res[1])); // without cache every resolve is executed
        CHECK(res[0] == res[1]); // but result is the same
    }

    SECTION("cache") {
        SECTION("no hints") {
            // Resolver will use cache by default, first time it is not in cache, async call
            b.run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            // in cache, so the call is sync
            b.run();
            test.run_nowait();
            CHECK(resolver->cache_size() == 1);
            CHECK(res[0].is(res[1]));
        }

        SECTION("both empty hints") {
            b.hints(AddrInfoHints());

            b.run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            b.run();
            test.run_nowait();
            CHECK(resolver->cache_size() == 1);
            CHECK(res[0].is(res[1]));
        }

        SECTION("custom hints and empty hints") {
            b.hints(AddrInfoHints(AF_INET));

            b.run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            b.hints(AddrInfoHints());
            b.run();
            test.run();
            CHECK(resolver->cache_size() == 2);
            CHECK(!res[0].is(res[1]));
        }

        SECTION("different hints") {
            b.hints(AddrInfoHints(AF_INET));

            b.run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            b.hints(AddrInfoHints(AF_INET6));
            b.run();
            test.run();
            CHECK(resolver->cache_size() == 2);
            CHECK(!res[0].is(res[1]));
        }
    }

    SECTION("service/port") {
        b.service("80");

        b.run();
        test.run();
        CHECK(resolver->cache_size() == 1);

        b.service("");
        b.port(80);
        b.run();
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
        auto b = resolver->resolve().node("localhost").on_resolve(cb);

        b.port(80);
        b.run();
        test.run();
        CHECK(resolver->cache_size() == 1);

        b.port(443);
        b.run();
        test.run();
        CHECK(resolver->cache_size() == 2);

        b.port(22);
        b.run();
        test.run();
        CHECK(resolver->cache_size() == 1);
    }

    SECTION("timeout") {
        // will make it in required time
        resolver->resolve("localhost", [&](ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            CHECK(!err);
            CHECK(ai);
        }, 1000);
        test.run();
        CHECK(resolver->cache_size() == 1);

        // will not make it
        resolver->resolve("ya.ru", [&](ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            REQUIRE(err);
            CHECK(err->code() == std::errc::timed_out);
            CHECK(!ai);
        }, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        test.run();
        CHECK(resolver->cache_size() == 1);
    }

    SECTION("cancel") {
        expected_cnt = 1;

        auto req = resolver->resolve("tut.by", [&](ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            REQUIRE(err);
            CHECK(err->code() == std::errc::operation_canceled);
            CHECK(!ai);
        });

        SECTION("sync") {
            req->cancel();
        }
        SECTION("async") {
            test.loop->delay([=]{
                req->cancel();
            });
        }
        test.run();

        req->cancel(); // should be no-op
    }

    auto cancel_cb = [&](ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
        test.happens("r");
        REQUIRE(err);
        CHECK(err->code() == std::errc::operation_canceled);
        CHECK(!ai);
    };

    SECTION("reset") {
        auto req = resolver->resolve("lenta.ru", cancel_cb);
        resolver->resolve("mail.ru", cancel_cb);

        SECTION("sync") {
            expected_cnt = 3;
            resolver->resolve("localhost", cancel_cb); // in async it may complete earlier than our loop->delay
            resolver->reset();
        }
        SECTION("async") {
            expected_cnt = 2;
            test.loop->delay([&]{
                resolver->reset();
            });
        }

        test.run();

        req->cancel(); // should not die
    }

    SECTION("resolver destroy") {
        auto req = resolver->resolve("lenta.ru", cancel_cb);
        resolver->resolve("mail.ru", cancel_cb);

        SECTION("sync") {
            expected_cnt = 3;
            resolver->resolve("localhost", cancel_cb); // in async it may complete earlier than our loop->delay
            resolver = nullptr;
        }
        SECTION("async") {
            expected_cnt = 2;
            test.loop->delay([&]{
                resolver = nullptr;
            });
        }

        test.run();

        req->cancel();
    }

    SECTION("loop destroy") {
        LoopSP loop = new Loop();
        resolver = new Resolver(loop);

        auto req = resolver->resolve("lenta.ru", cancel_cb);
        resolver->resolve("mail.ru", cancel_cb);

        SECTION("sync") {
            expected_cnt = 3;
            resolver->resolve("localhost", cancel_cb); // in async it may complete earlier than our loop->delay
            loop = nullptr;
        }
        SECTION("async") {
            expected_cnt = 2;
            test.loop->delay([&]{
                loop = nullptr;
            });
            test.run();
        }

        CHECK_THROWS(resolver->resolve("localhost", cancel_cb));
        resolver->reset();
        req->cancel();
    }

    while (expected_cnt-- > 0) test.expected.push_back("r");

    test.loop->dump();

}
