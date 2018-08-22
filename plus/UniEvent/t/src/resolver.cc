#include <catch.hpp>

#include <chrono>
#include <thread>
#include <sstream>

#include <panda/unievent/test/AsyncTest.h>

#include <panda/unievent/CachedResolver.h>
#include <panda/unievent/Resolver.h>
#include <panda/unievent/Timer.h>
#include <panda/unievent/TCP.h>
#include <panda/string.h>
#include <panda/refcnt.h>
#include <panda/log.h>

#include "test.h"


using namespace panda;

#ifdef TEST_RESOLVER

//#define _DEBUG 1

#ifdef _DEBUG
#define _DBG(x) do { std::cerr << "[test-resolver]" << x << std::endl; } while (0)
#else
#define _DBG(x)
#endif

std::string dump(addrinfo* ai) {
    std::stringstream ss;
    for(auto current = ai; current; current = current->ai_next) {
        ss << unievent::to_string(current) << "\n";
    }
    return ss.str();
}

TEST_CASE("simple resolver", "[resolver]") {
    unievent::test::AsyncTest test(500, {"resolved"});
    iptr<unievent::Resolver> resolver(new unievent::Resolver(test.loop));
    resolver->resolve("localhost",
            "80",
            nullptr,
            [&](addrinfo*, const unievent::ResolveError& err, bool){
                REQUIRE(!err);
            });

    test.await(resolver->resolve_event, "resolved");
}

TEST_CASE("cached resolver", "[resolver]") {
    unievent::test::AsyncTest test(500, {"resolved"});
    iptr<unievent::CachedResolver> resolver(new unievent::CachedResolver);
    unievent::ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "80",
            nullptr,
            [&](addrinfo*, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(!from_cache);
            });

    test.await(request1->event, "resolved");

    bool called = false;
    unievent::ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "80",
            nullptr,
            [&](addrinfo*, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(from_cache);
                called = true;
            });

    REQUIRE(called);
}

TEST_CASE("cached resolver, same hints", "[resolver]") {
    unievent::test::AsyncTest test(500, {"resolved"});
    iptr<unievent::CachedResolver> resolver(new unievent::CachedResolver);

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addrinfo1;
    unievent::ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](addrinfo* ai, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(!from_cache);
                addrinfo1 = ai;
            });

    test.await(request1->event, "resolved");

    _DBG(dump(addrinfo1));

    bool called = false;
    unievent::ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](addrinfo*, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(from_cache);
                called = true;
            });

    REQUIRE(called);
}

TEST_CASE("cached resolver, with hints and without hints", "[resolver]") {
    unievent::test::AsyncTest test(500, {"resolved1", "resolved2"});
    iptr<unievent::CachedResolver> resolver(new unievent::CachedResolver);

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    unievent::ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](addrinfo*, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(!from_cache);
            });

    test.await(request1->event, "resolved1");

    unievent::ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "80",
            nullptr,
            [&](addrinfo*, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(!from_cache);
            });

    test.await(request2->event, "resolved2");
}

TEST_CASE("cached resolver, with hints and with different hints", "[resolver]") {
    unievent::test::AsyncTest test(500, {"resolved1", "resolved2"});
    iptr<unievent::CachedResolver> resolver(new unievent::CachedResolver);

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    unievent::ResolveRequestSP request1 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](addrinfo*, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(!from_cache);
            });

    test.await(request1->event, "resolved1");

    hints.ai_family = AF_INET6;
    unievent::ResolveRequestSP request2 = resolver->resolve(test.loop,
            "localhost",
            "80",
            &hints,
            [&](addrinfo*, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(!from_cache);
            });

    test.await(request2->event, "resolved2");
}

TEST_CASE("standalone cached resolver", "[resolver]") {
    unievent::test::AsyncTest test(500, {"resolved"});

    iptr<unievent::CachedResolver> resolver(new unievent::CachedResolver);

    addrinfo* addrinfo1;
    unievent::ResolveRequestSP request1 = resolver->resolve(test.loop,
            "yandex.ru",
            "80",
            nullptr,
            [&](addrinfo* ai, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(!from_cache);
                CHECK(ai);
                addrinfo1 = ai;
            });

    test.await(request1->event, "resolved");

    string addr1 = unievent::to_string(addrinfo1);

    bool called = false;
    addrinfo* addrinfo2;
    unievent::ResolveRequestSP request2 = resolver->resolve(test.loop,
            "yandex.ru",
            "80",
            nullptr,
            [&](addrinfo* ai, const unievent::ResolveError& err, bool from_cache){
                REQUIRE(!err);
                CHECK(from_cache);
                CHECK(ai);
                addrinfo2 = ai;
                called = true;
            });

    REQUIRE(called);

    string addr2 = unievent::to_string(addrinfo2);

    // cached or not - the result is the same
    // will rotate for tcp connection only
    REQUIRE(addr1 == addr2);

    _DBG("ADDR " << addr1 << " " << addr2);
    
    _DBG("ADDR1 ***\n" << dump(addrinfo1));
    _DBG("ADDR2 ***\n" << dump(addrinfo2));
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

    iptr<unievent::cached_resolver::AddressRotator> address_rotator(new unievent::cached_resolver::AddressRotator(ai));
    addrinfo* next1 = address_rotator->next();
    addrinfo* next2 = address_rotator->next();

    std::set<addrinfo*> result;
    for(auto i=0;i<size*2;++i) {
        result.insert(address_rotator->next());
    }

    REQUIRE(result.size() == size);
}

TEST_CASE("cached resolver limit", "[resolver]") {
    size_t LIMIT = 2;
    iptr<unievent::Loop> loop(new unievent::Loop);
    iptr<unievent::CachedResolver> resolver(new unievent::CachedResolver(500, LIMIT));

    bool called = false;
    panda::iptr<panda::unievent::ResolveRequest> request = resolver->resolve(loop.get(),
            "localhost",
            "80",
            nullptr,
            [&](addrinfo*, const panda::unievent::ResolveError&, bool){
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
            [&](addrinfo*, const panda::unievent::ResolveError&, bool){
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
            [&](addrinfo*, const panda::unievent::ResolveError&, bool){
                called = true;
            });

    loop->run();

    REQUIRE(called);
    REQUIRE(resolver->cache_size() == 1);
}

#endif
