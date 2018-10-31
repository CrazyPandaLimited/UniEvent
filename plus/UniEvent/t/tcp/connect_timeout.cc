#include "../lib/test.h"
#include <thread>

TEST_CASE("connect to nowhere", "[tcp-connect-timeout][v-ssl][v-socks]") {
    AsyncTest test(10000, {"connected", "reset"});

    auto sa = test.get_refused_addr();
    size_t counter = 0;

    TCPSP client = make_client(test.loop);
    TimerSP timer = new Timer(test.loop);
    timer->timer_event.add([&](Timer*) {
        test.loop->stop();
    });

    client->connect().to(sa);
    client->write("123");

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        switch (counter) {
        case 0:
            _EDEBUG("0");
            test.happens("connected");
            counter++;
            client->connect().to(sa);
            client->write("123");
            break;
        case 1:
            _EDEBUG("1");
            test.happens("reset");
            counter++;
            client->reset();
            client->connect().to(sa);
            break;
        default:
            _EDEBUG("default %d", client->refcnt());
            timer->once(10); // 100ms for close_reinit
            break;
        }
    });
    test.run();
    _EDEBUG("END %d", client->refcnt());
}

TEST_CASE("connect timeout with real connection", "[tcp-connect-timeout][v-ssl][v-socks]") {
    AsyncTest test(250, {"connected1", "connected2"});

    TCPSP server = make_server(test.loop);
    auto sa = server->get_sockaddr();

    bool cached_resolver;
    SECTION("ordinary resolver") { cached_resolver = false; }
    SECTION("cached resolver")   { cached_resolver = true;  }

    TCPSP client = make_client(test.loop, cached_resolver);

    client->connect().to(sa).timeout(20);

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        if (err) WARN(err->what());
        CHECK_FALSE(err);
    });

    test.await(client->connect_event, "connected1");

    client->disconnect();

    client->connect().to(sa).timeout(20);

    test.await(client->connect_event, "connected2");

    REQUIRE(test.await_not(client->connect_event, 20));
}

TEST_CASE("connect timeout with real canceled connection", "[tcp-connect-timeout][v-ssl]") {
    int connected = 0;
    int errors = 0;
    int successes = 0;
    int tries = getenv("TEST_FULL") ? (variation.ssl ? 200 : 4000) : (variation.ssl ? 50 : 100);

    AsyncTest test(50000, {"connected1", "connected2"});
    TCPSP server = make_server(test.loop);
    auto sa = server->get_sockaddr();
    server->connection_event.add([](Stream*, Stream*, const CodeError*) {});

    TCPSP clients[tries];
    std::vector<decltype(clients[0]->connect_event)*> disps;

    for (int i = 0; i < tries; ++i) {
        bool cached_resolver = i % 2;
        _EDEBUG("----------------- first %d, %d -----------------", i, cached_resolver);

        auto client = clients[i] = make_client(test.loop, cached_resolver);

        client->connect().to(sa).timeout(10);

        client->connect_event.add([&, i](Stream*, const CodeError* err, ConnectRequest*) {
            _EDEBUG("----------------- connect event");
            //if (err) {WARN(i); WARN(err->what());}
            ++connected;
            err ? ++errors : ++successes;
        });

        disps.push_back(&client->connect_event);
    }

    test.await(disps, "connected1");
    clear_resolver_cache(test.loop);
    _EDEBUG("----------------- second -----------------");

    for (int i = 0; i < tries; ++i) {
        clients[i]->disconnect();
        clients[i]->connect().to(sa).timeout(10000);
    }

    test.await(disps, "connected2");

    clear_resolver_cache(test.loop);

    CHECK(connected == tries * 2);
    CHECK(successes >= tries);
    // NB some connections could be made nevertheless canceled
}

TEST_CASE("connect timeout with black hole", "[tcp-connect-timeout][v-ssl][v-socks]") {
    AsyncTest test(150, {"connected called"});

    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    TCPSP client = make_client(test.loop, cached_resolver);
    client->connect().to(test.get_blackhole_addr()).timeout(10);

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err->whats() != "");
    });
    test.await(client->connect_event, "connected called");
}

TEST_CASE("connect timeout clean queue", "[tcp-connect-timeout][v-ssl][v-socks]") {
    AsyncTest test(250, {"connected called"});
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    TCPSP client = make_client(test.loop, cached_resolver);
    client->connect().to(test.get_blackhole_addr()).timeout(10);

    client->write("123");

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err->whats() != "");
    });
    test.await(client->connect_event, "connected called");
    REQUIRE(test.await_not(client->write_event, 10));
}

TEST_CASE("connect timeout with black hole in roll", "[tcp-connect-timeout][v-ssl][v-socks]") {
    AsyncTest test(1000, {"done"});

    TCPSP client = make_client(test.loop);
    client->connect().to(test.get_blackhole_addr()).timeout(10);

    size_t counter = 5;
    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err->whats() != "");
        if (--counter > 0) {
            client->connect().to(test.get_blackhole_addr()).timeout(10);
            SECTION("usual") {}
            SECTION("sleep") {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } else {
            test.loop->stop();
        }
    });
    test.run();
    test.happens("done");
}

TEST_CASE("regression on not cancelled timer in second (sync) connect", "[tcp-connect-timeout][v-ssl][v-socks]") {
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    AsyncTest test(250, {"not_connected1", "not_connected2"});
    auto sa = test.get_refused_addr();

    TCPSP client = make_client(test.loop, cached_resolver);

    client->connect().to(sa).timeout(100);

    bool failed = false;
    bool called = false;
    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        failed = err;
        called = true;
    });

    test.await(client->connect_event, "not_connected1");

    REQUIRE(called);
    REQUIRE(failed);

    clear_resolver_cache(test.loop);
    called = false;
    failed = false;

    client->connect().to(sa).timeout(100);

    test.await(client->connect_event, "not_connected2");

    REQUIRE(called);
    REQUIRE(failed);
}
