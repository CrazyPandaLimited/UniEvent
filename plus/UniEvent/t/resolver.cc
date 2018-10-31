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
    ResolverSP resolver{new Resolver(loop)};
    resolver->resolve("google.com", "", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { REQUIRE(!err); });
    loop->run();
}

TEST_CASE("cached resolver", "[resolver]") {
    LoopSP loop(new Loop);
    CachedResolverSP resolver{new CachedResolver(loop)};

    // it is not in cache, async call
    resolver->resolve("localhost", "", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { REQUIRE(!err); });

    loop->run();

    // in cache, so the call is sync
    bool called = false;
    resolver->resolve("localhost", "", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
        REQUIRE(!err);
        called = true;
    });

    REQUIRE(called);
}

TEST_CASE("cached resolver, same hints", "[resolver]") {
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(loop));

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addrinfo1;
    resolver->resolve("localhost", "80", &hints, [&](ResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
        REQUIRE(!err);
        addrinfo1 = address->head;
    });

    loop->run();

    bool called = false;
    resolver->resolve("localhost", "80", &hints, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) {
        REQUIRE(!err);
        called = true;
    });

    REQUIRE(called);
}

TEST_CASE("cached resolver, with hints and without hints", "[resolver]") {
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(loop));

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    resolver->resolve("localhost", "80", &hints, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { REQUIRE(!err); });

    loop->run();

    resolver->resolve("localhost", "80", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { REQUIRE(!err); });

    loop->run();
}

TEST_CASE("cached resolver, with hints and with different hints", "[resolver]") {
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(loop));

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    resolver->resolve("localhost", "80", &hints, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { REQUIRE(!err); });

    loop->run();

    hints.ai_family = AF_INET6;
    resolver->resolve("localhost", "80", &hints, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError* err) { REQUIRE(!err); });

    loop->run();
}

TEST_CASE("standalone cached resolver", "[resolver]") {
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(loop));

    SockAddr addr1;
    resolver->resolve("yandex.ru", "80", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
        REQUIRE(!err);
        CHECK(address->head);
        addr1 = address->head->ai_addr;
    });

    loop->run();

    bool called = false;
    SockAddr addr2;
    resolver->resolve("yandex.ru", "80", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP address, const CodeError* err) {
        REQUIRE(!err);
        CHECK(address->head);
        addr2  = address->head->ai_addr;
        called = true;
    });

    REQUIRE(called);

    // cached or not - the result is the same
    // will rotate for tcp connection only
    REQUIRE(addr1 == addr2);
}

TEST_CASE("rotator", "[resolver]") {
    int size = 10;
    addrinfo ai[size];
    for(auto i=0;i<size;++i) {
        memset(&ai[i], 0, sizeof(addrinfo));
        if(i < size-1) {
            ai[i].ai_next = &ai[i+1];
        }
    }

    AddressRotatorSP address_rotator(new AddressRotator(ai));

    std::set<addrinfo*> result;
    for(auto i=0;i<size;++i) {
        result.insert(address_rotator->rotate());
    }
    
    // prevent from freeing stack allocated addrinfo 
    address_rotator->detach();

    REQUIRE(result.size() == size);
}

TEST_CASE("cached resolver limit", "[resolver]") {
    size_t LIMIT = 2;
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(loop, 500, LIMIT));

    bool called = false;
    resolver->resolve("localhost", "80", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });

    loop->run();

    REQUIRE(called);

    REQUIRE(resolver->cache_size() == 1);

    called = false;
    resolver->resolve("google.com", "80", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });

    loop->run();

    REQUIRE(called);
    REQUIRE(resolver->cache_size() == 2);

    called = false;
    resolver->resolve("yandex.ru", "80", nullptr, [&](ResolverSP, ResolveRequestSP, AddrInfoSP, const CodeError*) { called = true; });

    loop->run();

    REQUIRE(called);
    REQUIRE(resolver->cache_size() == 1);
}
