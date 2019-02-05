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
    AsyncTest test(2000, {"r", "r"});
    ResolverSP resolver = new Resolver(test.loop);
    AddrInfo res;

    SECTION("basic") {
        resolver->resolve().node("google.com").use_cache(false).on_resolve([&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            CHECK(!err);
            CHECK(ai);
            res = ai;
        }).run();
        test.run();

        resolver->resolve().node("google.com").use_cache(false).on_resolve([&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            CHECK(!err);
            CHECK(!ai.is(res)); // without cache every resolve is executed
        }).run();
        test.run();
    }

    SECTION("cached") {
        SECTION("no hints") {
            // Resolver will use cache by default, first time it is not in cache, async call
            resolver->resolve("google.com", [&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
                test.happens("r");
                REQUIRE(!err);
                res = ai;
            });
            test.run();
            CHECK(resolver->cache_size() == 1);

            // in cache, so the call is sync
            resolver->resolve("google.com", [&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
                test.happens("r");
                REQUIRE(!err);
                REQUIRE(ai.is(res));
            });
            test.run_nowait();
            CHECK(resolver->cache_size() == 1);
        }

        SECTION("both default hints") {
            auto b = resolver->resolve().node("google.com").service("80").on_resolve([&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
                test.happens("r");
                REQUIRE(!err);
                res = ai;
            });
            b.hints(AddrInfoHints());

            b.run();
            test.run();
            CHECK(resolver->cache_size() == 1);

            b.run();
            test.run_nowait();
            CHECK(resolver->cache_size() == 1);
        }

    }

}


//TEST_CASE("cached resolver, same hints", "[resolver]") {
//    LoopSP loop(new Loop);
//    ResolverSP resolver(new Resolver(loop));
//    AddrInfoHintsSP hints = new AddrInfoHints();
//
//    bool called = false;
//    resolver->resolve("google.com", "80", hints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
//        REQUIRE(!err);
//        called = true;
//    });
//
//    loop->run();
//
//    REQUIRE(called);
//
//    called = false;
//    resolver->resolve("google.com", "80", hints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
//        REQUIRE(!err);
//        called = true;
//    });
//
//    loop->run_nowait();
//
//    REQUIRE(called);
//}
//
//TEST_CASE("cached resolver, with custom hints and default hints", "[resolver]") {
//    LoopSP loop(new Loop);
//    ResolverSP resolver(new Resolver(loop));
//
//    bool called = false;
//    resolver->resolve("localhost", "80", new AddrInfoHints(AF_INET), [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
//        REQUIRE(!err);
//        called = true;
//    });
//
//    loop->run();
//    REQUIRE(called);
//
//    called = false;
//    resolver->resolve("localhost", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
//        REQUIRE(!err);
//        called = true;
//    });
//
//    loop->run();
//    REQUIRE(called);
//}
//
//TEST_CASE("cached resolver, with hints and with different hints", "[resolver]") {
//    LoopSP loop(new Loop);
//    ResolverSP resolver(new Resolver(loop));
//
//    bool called = false;
//    resolver->resolve("google.com", "80", new AddrInfoHints(AF_INET), [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
//        REQUIRE(!err);
//        called = true;
//    });
//
//    loop->run();
//    REQUIRE(called);
//
//    called = false;
//    resolver->resolve("google.com", "80", new AddrInfoHints(AF_INET6), [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
//        REQUIRE(!err);
//        called = true;
//    });
//
//    loop->run();
//    REQUIRE(called);
//}
//
//TEST_CASE("standalone cached resolver", "[resolver]") {
//    LoopSP loop(new Loop);
//    ResolverSP resolver(new Resolver(loop));
//
//    SockAddr addr1;
//    resolver->resolve("yandex.ru", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
//        REQUIRE(!err);
//        CHECK(address->head);
//        addr1 = address->head->ai_addr;
//    });
//
//    loop->run();
//
//    bool called = false;
//    SockAddr addr2;
//    resolver->resolve("yandex.ru", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
//        REQUIRE(!err);
//        std::string addr_str = address->to_string();
//        CHECK(address->head);
//        addr2  = address->head->ai_addr;
//        called = true;
//    });
//
//    loop->run_nowait();
//    REQUIRE(called);
//
//    // cached or not - the result is the same
//    REQUIRE(addr1 == addr2);
//}
//
//TEST_CASE("cached resolver limit", "[resolver]") {
//    size_t LIMIT = 2;
//    LoopSP loop(new Loop);
//    ResolverSP resolver(new Resolver(loop, 500, LIMIT));
//
//    bool called = false;
//    resolver->resolve("localhost", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });
//
//    loop->run();
//
//    REQUIRE(called);
//
//    REQUIRE(resolver->cache_size() == 1);
//
//    called = false;
//    resolver->resolve("google.com", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });
//
//    loop->run();
//
//    REQUIRE(called);
//    REQUIRE(resolver->cache_size() == 2);
//
//    called = false;
//    resolver->resolve("yandex.ru", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });
//
//    loop->run();
//
//    REQUIRE(called);
//    REQUIRE(resolver->cache_size() == 1);
//}
//
//TEST_CASE("resolve connect timeout", "[tcp-connect-timeout][v-ssl]") {
//    AsyncTest test(5000, {});
//
//    TCPSP server = make_server(test.loop);
//    auto  sa     = server->get_sockaddr();
//
//    for (size_t i = 0; i < 50; ++i) {
//        TCPSP client = make_client(test.loop, false);
//
//        test.loop->update_time();
//        client->connect().to(sa.ip(), sa.port()).timeout(1);
//
//        client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) { CHECK(err); });
//
//        for (size_t i = 0; i < 10; ++i) {
//            client->write("123");
//        }
//        std::this_thread::sleep_for(std::chrono::milliseconds(2));
//
//        test.await(client->connect_event, "");
//    }
//}
