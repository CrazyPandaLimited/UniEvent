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
