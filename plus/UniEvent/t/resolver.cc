#include "lib/test.h"
#include <thread>
#include <sstream>
#include <set>
#include <panda/log.h>

namespace {
    static int dcnt = 0;
    static int ccnt = 0;

    struct MyLoop : Loop {
        MyLoop  () { ++ccnt; }
        ~MyLoop () override { ++dcnt; }
    };

    struct MyResolver : Resolver {
        MyResolver (const LoopSP& loop) : Resolver(loop) { ++ccnt; }
        ~MyResolver () { ++dcnt; }
    };
}

TEST_CASE("resolver", "[resolver]") {
    AsyncTest test(2000, {});
    ResolverSP resolver = new Resolver(test.loop);
    std::vector<AddrInfo> res;
    auto success_cb = [&](const AddrInfo& ai, const CodeError& err, const Resolver::RequestSP&) {
        test.happens("r");
        CHECK(!err);
        CHECK(ai);
        res.push_back(ai);
    };
    auto noop_cb = [&](const AddrInfo&, const CodeError&, const Resolver::RequestSP&) {};
    auto req = resolver->resolve()->node("localhost")->on_resolve(success_cb);
    int expected_cnt = 2;
    auto full = getenv("TEST_FULL");

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

            req->hints(AddrInfoHints(AF_INET, SOCK_STREAM));
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
        resolver->resolve("localhost", [&](const AddrInfo& ai, const CodeError& err, const Resolver::RequestSP&) {
            test.happens("r");
            CHECK(!err);
            CHECK(ai);
        }, 1000);
        test.run();
        CHECK(resolver->cache_size() == 1);

        // will not make it
        resolver->resolve("ya.ru", [&](const AddrInfo& ai, const CodeError& err, Resolver::RequestSP) {
            test.happens("r");
            CHECK(err.code() == std::errc::timed_out);
            CHECK(!ai);
        }, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        test.run();
        CHECK(resolver->cache_size() == 1);
    }

    auto canceled_cb = [&](const AddrInfo& ai, const CodeError& err, const Resolver::RequestSP&) {
        test.happens("r");
        CHECK(err.code() == std::errc::operation_canceled);
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
                    test.loop->delay([&]{
                        req->cancel();
                    });
                    req = resolver->resolve("localhost", canceled_cb);
                }
            }
        }

        SECTION("cached") {
            resolver->resolve("localhost", noop_cb);
            test.run();
            CHECK(resolver->cache_size() == 1);

            SECTION("sync") {
                req = resolver->resolve("localhost", canceled_cb);
                req->cancel();
            }
            SECTION("async") {
                test.loop->delay([&]{
                    req->cancel();
                });
                req = resolver->resolve("localhost", canceled_cb);
            }
        }

        test.run();

        req->cancel(); // should be no-op
    }

    SECTION("reset") {
        Resolver::RequestSP req;

        SECTION("not cached") {
            expected_cnt = 3;
            req = resolver->resolve("lenta.ru", canceled_cb);
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
        }

        SECTION("cached") {
            expected_cnt = 1;
            resolver->resolve("localhost", noop_cb);
            test.run();
            CHECK(resolver->cache_size() == 1);
            SECTION("sync") {
                req = resolver->resolve("localhost", canceled_cb);
                resolver->reset();
            }
            SECTION("async") {
                test.loop->delay([&]{
                    resolver->reset();
                });
                req = resolver->resolve("localhost", canceled_cb);
            }
        }

        test.run();
        req->cancel(); // should not die
    }

    ccnt = dcnt = 0;

    SECTION("hold resolver while active request") {
        expected_cnt = 1;
        resolver = new MyResolver(test.loop);
        resolver->resolve("localhost", [&test](const AddrInfo&, const CodeError& err, const Resolver::RequestSP& req) {
            test.happens("r");
            CHECK(!err);
            CHECK(req->resolver()->loop());
        });
        resolver = nullptr;
        CHECK(dcnt == 0);
        test.run();
        CHECK(dcnt == 1);
    }

    SECTION("hold loop while active request (for loop resolver)") {
        expected_cnt = 1;
        LoopSP loop = new MyLoop();
        Loop* l = loop.get();
        loop->resolver()->resolve("localhost", [&test](const AddrInfo&, const CodeError& err, const Resolver::RequestSP& req) {
            test.happens("r");
            CHECK(!err);
            CHECK(req->resolver()->loop()->resolver());
        });
        loop = nullptr;
        CHECK(dcnt == 0);
        l->run();
        CHECK(dcnt == 1);
    }

    SECTION("many requests") {
        string node;
        SECTION("local") {
            node = "localhost";
            SECTION("cached")     {}
            SECTION("not cached") { resolver->cache_limit(0); }
        }
        if (full) SECTION("remote") {
            node = "ya.ru";
            SECTION("cached")     {}
            SECTION("not cached") { resolver->cache_limit(0); }
        }
        expected_cnt = 50;
        for (int i = 0; i < expected_cnt; ++i) resolver->resolve(node, success_cb);
        test.run();
        CHECK(res.size() == expected_cnt);
    }

    SECTION("exception safety") {
        expected_cnt = 2;
        for (int i = 0; i < 2; ++i) {
            test.loop->resolver()->resolve("localhost", [&](auto, auto, auto) {
                test.happens("r");
                throw "epta";
            });
            REQUIRE_THROWS(test.run());
        }
    }

    SECTION("sync_resolve exception") {
        expected_cnt = 0;
        REQUIRE_THROWS( sync_resolve(test.loop->backend(), "sukanahblya", 12345) );
    }

    while (expected_cnt-- > 0) test.expected.push_back("r");
}
