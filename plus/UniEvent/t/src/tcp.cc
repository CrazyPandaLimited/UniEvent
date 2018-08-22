#include <catch.hpp>
#include <panda/unievent/test/AsyncTest.h>

#include <panda/unievent/Timer.h>
#include <panda/unievent/TCP.h>
#include <panda/string.h>
#include <chrono>

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

TEST_CASE("sync connect error", "[tcp]") {
    AsyncTest test(2000, {"error"});
    in_port_t port = find_free_port();
//    TCPSP server = make_server(port, test.loop);

    TCPSP client = new TCP(test.loop);
    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        REQUIRE(err);
        _pex_(client)->type = UV_TCP;

        SECTION("disconnect") {
            client->disconnect();
        }
        SECTION("just go") {
        }

    });

    client->connect("localhost", panda::to_string(port));
    _pex_(client)->type = UV_HANDLE; // make uv_handle invalid for sync error after resolving

    client->write("123");
    client->disconnect();

    auto res = test.await(client->write_event, "error");
    StreamError err = std::get<1>(res);
    REQUIRE(err.code() == ERRNO_ECANCELED);
}

TEST_CASE("write to closed socket", "[tcp]") {
    AsyncTest test(2000, {"error"});
    in_port_t port = find_free_port();
    TCPSP server = make_server(port, test.loop);

    TCPSP client = new TCP(test.loop);
    client->connect("localhost", panda::to_string(port));
    client->write("1");
    test.await(client->write_event);
    client->disconnect();

    if (false) { //TODO we need test params here
        sp<Timer> t = Timer::once(10, [](Timer*){}, test.loop);
        test.await(t->timer_event);
    }

    try {
        SECTION ("write") {
            client->write("2");
            client->write_event.add([&](Stream*, const StreamError& err, WriteRequest*) {
                REQUIRE(err.code() == ERRNO_EBADF);
                test.happens("error");
                test.loop->stop();
            });
        }
        SECTION ("shutdown") {
            client->shutdown();
            client->shutdown_event.add([&](Stream*, const StreamError& err, ShutdownRequest*) {
                REQUIRE(err.code() == ERRNO_ENOTCONN);
                test.happens("error");
                test.loop->stop();
            });
        }
        test.loop->run();
    } catch (StreamError& err) {
        FAIL(err.what());
    }
}

TEST_CASE("call_soon", "[prepare]") {
    AsyncTest test(200, {"call"});
    size_t count = 0;
    Prepare::call_soon([&]() {
        count++;
        if (count >= 2) {
            FAIL("called twice");
        }
        test.happens("call");
        test.loop->stop();
    }, test.loop);
    test.run();
    TimerSP timer = Timer::once(50, [&](Timer*){
        test.loop->stop();
    }, test.loop);
    REQUIRE(count == 1);
}

TEST_CASE("immediate disconnect", "[tcp]") {
    AsyncTest test(500, {});
    in_port_t port = find_free_port();
    TCPSP server;
    SECTION ("no server") {
    }
    SECTION ("with server") {
        server = make_server(port, test.loop);
    }

    TCPSP client = new TCP(test.loop);
    string body;
    for (size_t i = 0; i < 100; ++i)  {
        body += "0123456789";
    }
    size_t write_count = 0;
    client->connect_event.add([&](Stream*, const StreamError& err, ConnectRequest*) {
        client->connect_event.remove_all();
        client->connect("localhost", panda::to_string(port));

        for (size_t i = 0; i < 1200; ++i) {
            write_count++;
            client->write(body);
        }
        client->shutdown();
        client->disconnect();

    });

    size_t callback_count = 0;
    client->write_event.add([&](Stream*, const StreamError& err, WriteRequest*){
        callback_count++;
        if (callback_count == write_count) {
            test.loop->stop();
        }
    });

    client->connect("localhost", panda::to_string(port));

    test.run();
    REQUIRE(write_count == callback_count);
}

TEST_CASE("immidiate reset", "[tcp]") {
    AsyncTest test(2000, {"error"});
    in_port_t port = find_free_port();
    TCPSP server = make_server(port, test.loop);
    TCPSP client = new TCP(test.loop);
    client->use_ssl();

    client->connect("localhost", panda::to_string(port));
    client->reset();

    auto res = test.await(client->connect_event, "error");
    StreamError err = std::get<1>(res);
    REQUIRE(err.code() == ERRNO_ECANCELED);
}
