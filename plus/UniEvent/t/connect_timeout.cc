#include "lib/test.h"
#include <thread>

TEST_CASE("connect to nowhere", "[panda-event][timeout][socks][ssl]") {
    AsyncTest test(50000, {"connected", "reset"});

    auto port = find_free_port();
    size_t counter = 0;

    TCPSP client = make_client(test.loop);
    TimerSP timer = new Timer(test.loop);
    timer->timer_event.add([&](Timer*) {
        test.loop->stop();
    });

    client->connect().to("localhost", panda::to_string(port));
    client->write("123");

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        switch (counter) {
        case 0:
            _EDEBUG("0");
            test.happens("connected");
            counter++;
            client->connect().to("localhost", panda::to_string(port));
            client->write("123");
            break;
        case 1:
            _EDEBUG("1");
            test.happens("reset");
            counter++;
            client->reset();
            client->connect().to("localhost", panda::to_string(port));
            break;
        default:
            _EDEBUG("default %d", client->refcnt());
            timer->once(100); // 100ms for close_reinit
            break;
        }
    });
    test.run();
    _EDEBUG("END %d", client->refcnt());
}

TEST_CASE("connect no timeout with real connection", "[panda-event][timeout][socks][ssl]") {
    AsyncTest test(250, {"connected"});

    auto port = find_free_port();
    TCPSP server = make_server(port, test.loop);
    TCPSP client = make_client(test.loop);
    
    client->connect("localhost", panda::to_string(port));

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(!err);
    });
    test.await(client->connect_event, "connected");
    REQUIRE(test.await_not(client->connect_event, 110));
}

TEST_CASE("connect timeout with real connection", "[panda-event][timeout][socks][ssll]") {
    AsyncTest test(250, {"connected1", "connected2"});

    auto port = find_free_port();
    TCPSP server = make_server(port, test.loop);
    
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }
    
    TCPSP client = make_client(test.loop, cached_resolver);
    
    client->connect()
                .to("localhost", panda::to_string(port))
                .timeout(100);

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(!err);
    });

    test.await(client->connect_event, "connected1");
   
    client->disconnect();

    client->connect()
                .to("localhost", panda::to_string(port))
                .timeout(100);

    test.await(client->connect_event, "connected2");

    REQUIRE(test.await_not(client->connect_event, 110));
}

TEST_CASE("connect timeout with real canceled connection", "[panda-event][timeout][socks][ssl]") {
    int connected = 0;
    int errors = 0;
    int successes = 0;
    int tries = 1000;
    auto port = find_free_port();
    
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    for (int i = 0; i < tries; ++i) {
        _EDEBUG("----------------- first %d, %d -----------------", i, cached_resolver);
        AsyncTest test(1000, {"connected1", "connected2"});

        TCPSP server = make_server(port, test.loop);
        TCPSP client = make_client(test.loop, cached_resolver);

        client->connect()
            .to("localhost", panda::to_string(port))
            .timeout(1);

        bool success;
        client->connect_event.add(
            [&](Stream*, const CodeError* err, ConnectRequest*) {
                _EDEBUG("----------------- connect event");
                success = !err;
                ++connected;
                err ? ++errors : ++successes;
            });

        test.await(client->connect_event, "connected1");
        clear_resolver_cache();
        _EDEBUG("----------------- second -----------------");

        client->connect()
            .to("localhost", panda::to_string(port))
            .timeout(1000);

        test.await(client->connect_event, "connected2");

        if(!success) {
            FAIL("call failed");
        }

        clear_resolver_cache();
    }

    REQUIRE(connected == tries * 2);
    // NB some connections could be made nevertheless canceled 
}

//TEST_CASE("connect timeout with error connection", "[timeout]") {
    //AsyncTest test(250, {});

    //auto port = panda::to_string(find_free_port());
    //bool cached_resolver;
    //SECTION("ordinary resolver") {
        //cached_resolver = false;
    //}
    //SECTION("cached resolver") {
        //cached_resolver = true;
    //}

    //TCPSP client = new TCP(test.loop);

    //client->connect()
        //.to("localhost", panda::to_string(1))
        //.cached_resolver(cached_resolver)
        //.timeout(100);

    //size_t counter = 10;
    //client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        //CHECK(err);
        //while (true) {
            //try {
                //if (--counter) {
                    //client->connect().to("localhost", port);
                //} else {
                    //test.loop->stop();
                //}
                //break;
            //} catch (Error& err) {
                //continue;
            //}
        //}

    //});
    //test.run();
    //REQUIRE(test.await_not(client->connect_event, 110));
//}


TEST_CASE("connect timeout with black hole", "[panda-event][timeout][socks][ssl]") {
    AsyncTest test(150, {"connected called"});
    
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    TCPSP client = make_client(test.loop, cached_resolver);
    client->connect()
        .to("google.com", "81")  // black hole
        .timeout(100);

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err->whats() != "");
    });
    test.await(client->connect_event, "connected called");
}

TEST_CASE("connect timeout clean queue", "[panda-event][timeout][socks][ssl]") {
    AsyncTest test(250, {"connected called"});
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    TCPSP client = make_client(test.loop, cached_resolver);
    client->connect()
        .to("google.com", "81")  // black hole
        .timeout(100);

    client->write("123");

    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err->whats() != "");
    });
    test.await(client->connect_event, "connected called");
    REQUIRE(test.await_not(client->write_event, 100));
}

TEST_CASE("connect timeout with black hole in roll", "[panda-event][timeout][socks][ssl]") {
    AsyncTest test(1000, {"done"});

    TCPSP client = make_client(test.loop);
    client->connect().to("google.com", "81") // black hole
                     .timeout(50);

    size_t counter = 5;
    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err->whats() != "");
        if (--counter > 0) {
            client->connect().to("google.com", "81") // black hole
                             .timeout(50);
            SECTION("usual") {
            }
            SECTION("sleep") {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            test.loop->stop();
        }
    });
    test.run();
    test.happens("done");
}

TEST_CASE("regression on not cancelled timer in second (sync) connect", "[panda-event][timeout][socks][ssl]") {
    auto port = find_free_port();
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    AsyncTest test(250, {"not_connected1", "not_connected2"});

    TCPSP client = make_client(test.loop, cached_resolver);

    client->connect().to("localhost", panda::to_string(port))
            .timeout(100);

    bool failed = false;
    bool called = false;
    client->connect_event.add(
        [&](Stream*, const CodeError* err, ConnectRequest*) {
            failed = err;
            called = true;
        });

    test.await(client->connect_event, "not_connected1");

    REQUIRE(called);
    REQUIRE(failed);
    
    clear_resolver_cache();
    called = false;
    failed = false;

    client->connect().to("localhost", panda::to_string(port))
        .timeout(100);

    test.await(client->connect_event, "not_connected2");
    
    REQUIRE(called);
    REQUIRE(failed);
}
