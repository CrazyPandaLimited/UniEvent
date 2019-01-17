#include "lib/test.h"
#include <thread>
#include <sstream>
#include <set>

std::string dump(addrinfo* ai) {
    std::stringstream ss;
    for(auto current = ai; current; current = current->ai_next) {
        ss << SockAddr(ai->ai_addr) << "\n";
    }
    return ss.str();
}

TEST_CASE("basic resolver", "[resolver]") {
    LoopSP loop(new Loop);
    SimpleResolverSP resolver{new SimpleResolver(loop)};
    resolver->resolve("google.com", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { 
        _EDEBUG("%p", err);
        REQUIRE(!err); 
    });
    loop->run();
    //resolver->stop();
}

TEST_CASE("cached resolver", "[resolver]") {
    LoopSP loop(new Loop);
    ResolverSP resolver{new Resolver(loop)};

    // Resolver will use cache by default, first time it is not in cache, async call
    bool called = false;
    resolver->resolve("google.com", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { 
        REQUIRE(!err); 
        called = true;
    });

    loop->run();
    
    REQUIRE(called);

    // in cache, so the call is sync
    called = false;
    resolver->resolve("google.com", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
        REQUIRE(!err);
        called = true;
    });
    loop->run_nowait();

    REQUIRE(called);
}

TEST_CASE("cached resolver, same hints", "[resolver]") {
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));
    AddrInfoHintsSP hints = new AddrInfoHints();
    
    bool called = false;
    resolver->resolve("google.com", "80", hints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
        REQUIRE(!err);
        called = true;
    });

    loop->run();
    
    REQUIRE(called);

    called = false;
    resolver->resolve("google.com", "80", hints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
        REQUIRE(!err);
        called = true;
    });

    loop->run_nowait();

    REQUIRE(called);
}

TEST_CASE("cached resolver, with custom hints and default hints", "[resolver]") {
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));

    bool called = false;
    resolver->resolve("localhost", "80", new AddrInfoHints(AF_INET), [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { 
        REQUIRE(!err); 
        called = true;
    });
    
    loop->run();
    REQUIRE(called);
    
    called = false;
    resolver->resolve("localhost", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { 
        REQUIRE(!err); 
        called = true;
    });

    loop->run();
    REQUIRE(called);
}

TEST_CASE("cached resolver, with hints and with different hints", "[resolver]") {
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));

    bool called = false;
    resolver->resolve("google.com", "80", new AddrInfoHints(AF_INET), [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { 
        REQUIRE(!err);
        called = true;
    });

    loop->run();
    REQUIRE(called);

    called = false;
    resolver->resolve("google.com", "80", new AddrInfoHints(AF_INET6), [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { 
        REQUIRE(!err); 
        called = true;
    });

    loop->run();
    REQUIRE(called);
}

TEST_CASE("standalone cached resolver", "[resolver]") {
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop));

    SockAddr addr1;
    resolver->resolve("yandex.ru", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
        REQUIRE(!err);
        CHECK(address->head);
        addr1 = address->head->ai_addr;
    });

    loop->run();

    bool called = false;
    SockAddr addr2;
    resolver->resolve("yandex.ru", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
        REQUIRE(!err);
        std::string addr_str = address->to_string();
        CHECK(address->head);
        addr2  = address->head->ai_addr;
        called = true;
    });

    loop->run_nowait();
    REQUIRE(called);

    // cached or not - the result is the same
    REQUIRE(addr1 == addr2);
}

TEST_CASE("cached resolver limit", "[resolver]") {
    size_t LIMIT = 2;
    LoopSP loop(new Loop);
    ResolverSP resolver(new Resolver(loop, 500, LIMIT));

    bool called = false;
    resolver->resolve("localhost", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });

    loop->run();

    REQUIRE(called);

    REQUIRE(resolver->cache_size() == 1);

    called = false;
    resolver->resolve("google.com", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });

    loop->run();

    REQUIRE(called);
    REQUIRE(resolver->cache_size() == 2);

    called = false;
    resolver->resolve("yandex.ru", "80", new AddrInfoHints, [&](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });

    loop->run();

    REQUIRE(called);
    REQUIRE(resolver->cache_size() == 1);
}

TEST_CASE("resolve connect timeout", "[tcp-connect-timeout][v-ssl]") {
    AsyncTest test(5000, {});

    TCPSP server = make_server(test.loop);
    auto  sa     = server->get_sockaddr();

    for (size_t i = 0; i < 50; ++i) {
        TCPSP client = make_client(test.loop, false);

        test.loop->update_time();
        client->connect().to(sa.ip(), sa.port()).timeout(1);

        client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) { CHECK(err); });

        for (size_t i = 0; i < 10; ++i) {
            client->write("123");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        test.await(client->connect_event, "");
    }
}

TEST_CASE("resolve cache moment invalidation", "[tcp-connect-timeout][v-ssl]") {
    AsyncTest test(500, {"cached", "resolved"});
    AddrInfoHintsSP hints = new AddrInfoHints;
    ResolverSP resolver = new Resolver(test.loop);

    auto no_callback = [](SimpleResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*){};

    ResolveRequestSP req = resolver->resolve("localhost", "80", hints, no_callback, true); // make sure, that localhost is in cache now
    test.await(req->event, "cached");

    req = resolver->resolve("localhost", "80", hints, no_callback, true); // real test resolving, should be get from cache and delayed with call_soon
    resolver->clear();
    auto res = test.await(req->event, "resolved");
    AddrInfoSP addr = std::get<2>(res);
    REQUIRE(addr);
}
