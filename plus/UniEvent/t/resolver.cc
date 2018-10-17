#include "lib/test.h"
#include <thread>
#include <sstream>
#include <set>

std::string dump(addrinfo* ai) {
    std::stringstream ss;
    for(auto current = ai; current; current = current->ai_next) {
        ss << to_string(current) << "\n";
    }
    return ss.str();
}

TEST_CASE("basic resolver", "[resolver]") {
    test::AsyncTest test(500, {"resolved"});
    ResolverSP resolver{new Resolver};
    ResolveRequestSP request = resolver->resolve(test.loop, 
            "localhost",
            "",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
            });

    test.await(request->event, "resolved");
}

TEST_CASE("cached resolver", "[resolver]") {
    test::AsyncTest test(500, {"resolved"});
    CachedResolverSP resolver{new CachedResolver};
    // it is not in cache, async call
    ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
            });

    test.await(request1->event, "resolved");

    bool called = false;
    // in cache, so the call is sync
    ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
                called = true;
            });

    REQUIRE(called);
}

TEST_CASE("cached resolver, same hints", "[resolver]") {
    test::AsyncTest test(500, {"resolved"});
    CachedResolverSP resolver(new CachedResolver);

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addrinfo1;
    ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP address, const CodeError* err){
                REQUIRE(!err);
                addrinfo1 = address->head;
            });

    test.await(request1->event, "resolved");

    bool called = false;
    ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
                called = true;
            });

    REQUIRE(called);
}

TEST_CASE("cached resolver, with hints and without hints", "[resolver]") {
    test::AsyncTest test(500, {"resolved1", "resolved2"});
    CachedResolverSP resolver(new CachedResolver);

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
            });

    test.await(request1->event, "resolved1");

    ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "80",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
            });

    test.await(request2->event, "resolved2");
}

TEST_CASE("cached resolver, with hints and with different hints", "[resolver]") {
    test::AsyncTest test(500, {"resolved1", "resolved2"});
    CachedResolverSP resolver(new CachedResolver);

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
            });

    test.await(request1->event, "resolved1");

    hints.ai_family = AF_INET6;
    ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError* err){
                REQUIRE(!err);
            });

    test.await(request2->event, "resolved2");
}

TEST_CASE("standalone cached resolver", "[resolver]") {
    test::AsyncTest test(500, {"resolved"});

    CachedResolverSP resolver(new CachedResolver);

    addrinfo* addrinfo1;
    ResolveRequestSP request1 = resolver->resolve(test.loop,
            "yandex.ru",
            "80",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP address, const CodeError* err){
                REQUIRE(!err);
                CHECK(address->head);
                addrinfo1 = address->head;
            });

    test.await(request1->event, "resolved");

    string addr1 = to_string(addrinfo1);

    bool called = false;
    addrinfo* addrinfo2;
    ResolveRequestSP request2 = resolver->resolve(test.loop,
            "yandex.ru",
            "80",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP address, const CodeError* err){
                REQUIRE(!err);
                CHECK(address->head);
                addrinfo2 = address->head;
                called = true;
            });

    REQUIRE(called);

    string addr2 = to_string(addrinfo2);

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
        result.insert(address_rotator->next());
    }
    
    // prevent from freeing stack allocated addrinfo 
    address_rotator->detach();

    REQUIRE(result.size() == size);
}

TEST_CASE("cached resolver limit", "[resolver]") {
    size_t LIMIT = 2;
    LoopSP loop(new Loop);
    CachedResolverSP resolver(new CachedResolver(500, LIMIT));

    bool called = false;
    ResolveRequestSP request = resolver->resolve(loop.get(),
            "localhost",
            "80",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                called = true;
            });

    loop->run();

    REQUIRE(called);

    REQUIRE(resolver->cache_size() == 1);

    called = false;
    request = resolver->resolve(loop.get(),
            "google.com",
            "80",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                called = true;
            });

    loop->run();

    REQUIRE(called);
    REQUIRE(resolver->cache_size() == 2);

    called = false;
    request = resolver->resolve(loop.get(),
            "yandex.ru",
            "80",
            nullptr,
            [&](AbstractResolverSP, ResolveRequestSP, BasicAddressSP, const CodeError*){
                called = true;
            });

    loop->run();

    REQUIRE(called);
    REQUIRE(resolver->cache_size() == 1);
}
