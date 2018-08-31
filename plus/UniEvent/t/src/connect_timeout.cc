#include <catch.hpp>
#include <panda/unievent/test/AsyncTest.h>

#include <panda/unievent/Timer.h>
#include <panda/unievent/TCP.h>
#include <panda/string.h>
#include <chrono>
#include <thread>

#include "test.h"

using namespace panda::unievent;
using test::AsyncTest;
using test::sp;

static TCPSP make_server(in_port_t port, Loop* loop) {
    TCPSP server = new TCP(loop);
    server->bind("localhost", panda::to_string(port));
    server->listen(1, [](Stream* serv, const StreamError&) {
        TCPSP client = new TCP(serv->loop());
        serv->accept(client);
    });
    return server;
}

#ifdef TEST_CONNECT_TIMEOUT

TEST_CASE("connect to nowhere", "[timeout]") {
    AsyncTest test(50000, {"connected", "reset"});

    auto port = find_free_port();
    size_t counter = 0;

    TCPSP client = new TCP(test.loop);
    TimerSP timer = new Timer(test.loop);
    timer->timer_event.add([&](Timer*) {
        test.loop->stop();
    });

    client->connect().to("localhost", panda::to_string(port));
    client->write("123");

    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        CHECK(err);
        switch (counter) {
        case 0:
            test.happens("connected");
            counter++;
            client->connect().to("localhost", panda::to_string(port));
            client->write("123");
            break;
        case 1:
            test.happens("reset");
            counter++;
            client->reset();
            client->connect().to("localhost", panda::to_string(port));
            break;
        default:
            timer->once(100); // 100ms for close_reinit
            break;
        }
    });
    test.run();
}

TEST_CASE("connect no timeout with real connection", "[timeout]") {
    AsyncTest test(250, {"connected"});

    auto port = find_free_port();
    TCPSP server = make_server(port, test.loop);
    TCPSP client = new TCP(test.loop);
    
    client->connect("localhost", panda::to_string(port));

    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        CHECK(!err);
    });
    test.await(client->connect_event, "connected");
    REQUIRE(test.await_not(client->connect_event, 110));
}

TEST_CASE("connect timeout with real connection", "[timeout]") {
    AsyncTest test(250, {"connected1", "connected2"});

    auto port = find_free_port();
    TCPSP server = make_server(port, test.loop);
    TCPSP client = new TCP(test.loop);
    
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }
    
    client->connect()
                .to("localhost", panda::to_string(port))
                .cached_resolver(cached_resolver)
                .timeout(100);

    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        CHECK(!err);
    });

    test.await(client->connect_event, "connected1");
   
    client->disconnect();

    client->connect()
                .to("localhost", panda::to_string(port))
                .cached_resolver(cached_resolver)
                .timeout(100);

    test.await(client->connect_event, "connected2");

    REQUIRE(test.await_not(client->connect_event, 110));
}

TEST_CASE("connect timeout with real canceled connection", "[timeout]") {
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
        AsyncTest test(250, {"connected1", "connected2"});

        TCPSP server = make_server(port, test.loop);
        TCPSP client = new TCP(test.loop);

        client->connect()
            .to("localhost", panda::to_string(port))
            .cached_resolver(cached_resolver)
            .timeout(1);

        bool success;
        client->connect_event.add(
            [&](Stream*, const StreamError& err, ConnectRequest*) {
                success = err.code() == 0;
                ++connected;
                err.code() ? ++errors : ++successes;
            });

        test.await(client->connect_event, "connected1");
        clear_resolver_cache();
        _EDEBUG("----------------- second -----------------");

        client->connect()
            .to("localhost", panda::to_string(port))
            .cached_resolver(cached_resolver)
            .timeout(1000);

        test.await(client->connect_event, "connected2");

        REQUIRE(success);

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
    //client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
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


TEST_CASE("connect timeout with black hole", "[timeout]") {
    AsyncTest test(150, {"connected called"});
    
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    TCPSP client = new TCP(test.loop);
    client->connect()
        .to("google.com", "81")  // black hole
        .cached_resolver(cached_resolver)
        .timeout(100);

    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err.whats() != "");
    });
    test.await(client->connect_event, "connected called");
}

TEST_CASE("connect timeout clean queue", "[timeout]") {
    AsyncTest test(250, {"connected called"});
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    TCPSP client = new TCP(test.loop);
    client->connect()
        .to("google.com", "81")  // black hole
        .cached_resolver(cached_resolver)
        .timeout(100);

    client->write("123");

    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err.whats() != "");
    });
    test.await(client->connect_event, "connected called");
    REQUIRE(test.await_not(client->write_event, 100));
}

TEST_CASE("connect timeout with black hole in roll", "[timeout]") {
    AsyncTest test(1000, {"done"});

    TCPSP client = new TCP(test.loop);
    client->connect().to("google.com", "81") // black hole
                     .timeout(50);

    size_t counter = 5;
    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        CHECK(err);
        REQUIRE(err.whats() != "");
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

TEST_CASE("regression on not cancelled timer in second (sync) connect", "[timeout]") {
    auto port = find_free_port();
    bool cached_resolver;
    SECTION("ordinary resolver") {
        cached_resolver = false;
    }
    SECTION("cached resolver") {
        cached_resolver = true;
    }

    AsyncTest test(250, {"not_connected1", "not_connected2"});

    TCPSP client = new TCP(test.loop);

    client->connect().to("localhost", panda::to_string(port))
            .cached_resolver(cached_resolver)
            .timeout(100);

    bool failed = false;
    bool called = false;
    client->connect_event.add(
        [&](Stream*, const StreamError& err, ConnectRequest*) {
            failed = err.code() != 0;
            called = true;
        });

    test.await(client->connect_event, "not_connected1");

    REQUIRE(called);
    REQUIRE(failed);
    
    clear_resolver_cache();
    called = false;
    failed = false;

    client->connect().to("localhost", panda::to_string(port))
        .cached_resolver(cached_resolver)
        .timeout(100);

    test.await(client->connect_event, "not_connected2");
    
    REQUIRE(called);
    REQUIRE(failed);
}

#endif
