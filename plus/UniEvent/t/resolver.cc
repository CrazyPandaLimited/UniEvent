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
    std::vector<AddrInfo> res;
    auto cb = [&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
        test.happens("r");
        CHECK(!err);
        CHECK(ai);
        res.push_back(ai);
    };
    auto b = resolver->resolve().node("localhost").on_resolve(cb);

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
        test.expected = {"r", "r", "r"};
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
        resolver->resolve("localhost", [&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            CHECK(!err);
            CHECK(ai);
        }, 1000);
        test.run();

        // will not make it
        resolver->resolve("ya.ru", [&](ResolverSP, ResolveRequestSP, const AddrInfo& ai, const CodeError* err) {
            test.happens("r");
            CHECK(err);
            CHECK(!ai);
        }, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        test.run();

        std::cout << "timeout end\n";
    }

}
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
