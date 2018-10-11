#include "lib/test.h"

TEST_CASE("sync connect error", "[panda-event][tcp][ssl]") {
    AsyncTest test(2000, {"error"});
    in_port_t port = find_free_port();
//    TCPSP server = make_server(port, test.loop);

    TCPSP client = make_client(test.loop);
    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
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
    auto err = std::get<1>(res);
    REQUIRE(err.code() == ERRNO_ECANCELED);
}

TEST_CASE("write to closed socket", "[panda-event][tcp][ssl]") {
    AsyncTest test(2000, {"error"});
    in_port_t port = find_free_port();
    TCPSP server = make_server(port, test.loop);

    TCPSP client = make_client(test.loop);
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
            client->write_event.add([&](Stream*, const CodeError* err, WriteRequest*) {
                REQUIRE(err);
                REQUIRE(err->code() == ERRNO_EBADF);
                test.happens("error");
                test.loop->stop();
            });
        }
        SECTION ("shutdown") {
            client->shutdown();
            client->shutdown_event.add([&](Stream*, const CodeError* err, ShutdownRequest*) {
                REQUIRE(err);
                REQUIRE(err->code() == ERRNO_ENOTCONN);
                test.happens("error");
                test.loop->stop();
            });
        }
        test.loop->run();
    } catch (CodeError* err) {
        FAIL(err->what());
    }
}

TEST_CASE("immediate disconnect", "[panda-event][tcp][ssl]") {
    AsyncTest test(500, {});
    in_port_t port1 = find_free_port();
    in_port_t port2 = find_free_port();
    TCPSP server1;
    TCPSP server2;
    SECTION ("no server") {
    }
    SECTION ("first no server second with server") {
        server2 = make_server(port2, test.loop);
    }
    SECTION ("with servers") {
        server1 = make_server(port1, test.loop);
        server2 = make_server(port2, test.loop);
    }

    TCPSP client = make_client(test.loop);
    string body;
    for (size_t i = 0; i < 100; ++i)  {
        body += "0123456789";
    }
    size_t write_count = 0;
    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        if(!err) {
            client->disconnect();
        }

        client->connect_event.remove_all();
        client->connect("localhost", panda::to_string(port2));

        for (size_t i = 0; i < 1200; ++i) {
            write_count++;
            client->write(body);
        }
        client->shutdown();
        client->disconnect();
    });

    size_t callback_count = 0;
    client->write_event.add([&](Stream*, const CodeError*, WriteRequest*){
        callback_count++;
        if (callback_count == write_count) {
            test.loop->stop();
        }
    });

    client->connect("localhost", panda::to_string(port1));

    test.run();
    REQUIRE(write_count == callback_count);
}

TEST_CASE("immediate client reset", "[panda-event][tcp][ssl]") {
    AsyncTest test(2000, {"error"});
    in_port_t port = find_free_port();
    TCPSP server;
    SECTION ("no server") {
    }
    SECTION ("with server") {
        server = make_server(port, test.loop);
    }
    SECTION ("with nossl server") {
        server = make_basic_server(port, test.loop);
    }
    TCPSP client = make_client(test.loop);

    client->connect("localhost", panda::to_string(port));
    client->reset();

    auto res = test.await(client->connect_event, "error");
    auto err = std::get<1>(res);
    REQUIRE(err.code() == ERRNO_ECANCELED);
}

TEST_CASE("immediate client write reset", "[panda-event][tcp][ssl]") {
    AsyncTest test(2000, {});
    in_port_t port = find_free_port();
    TCPSP server = make_server(port, test.loop);
    TCPSP client = make_client(test.loop);
    
    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
        REQUIRE_FALSE(err);
        client->reset();
        test.loop->stop();
    });

    client->connect("localhost", panda::to_string(port));
    client->write("123");

    test.loop->run();
}

TEST_CASE("reset accepted connection", "[panda-event][tcp][ssl]") {
    AsyncTest test(2000, {});
    in_port_t port = find_free_port();
    TCPSP server = make_server(port, test.loop);
    TCPSP client = make_client(test.loop);
    
    server->connection_event.add([&](Stream*, Stream* client, const CodeError* err) {
        REQUIRE_FALSE(err);
        client->reset();
        test.loop->stop();
    });

    client->connect("localhost", panda::to_string(port));

    test.loop->run();
}

TEST_CASE("try use server without certificate 1", "[panda-event][tcp][ssl]") {
    AsyncTest test(2000, {});
    in_port_t port = find_free_port();
    TCPSP server = new TCP(test.loop);
    server->bind("localhost", panda::to_string(port));
    server->listen(1);
    REQUIRE_THROWS(server->use_ssl());
}

TEST_CASE("try use server without certificate 2", "[panda-event][tcp][ssl]") {
    AsyncTest test(2000, {});
    in_port_t port = find_free_port();
    TCPSP server = new TCP(test.loop);
    server->bind("localhost", panda::to_string(port));
    server->use_ssl();
    REQUIRE_THROWS(server->listen(1));
}

TEST_CASE("server read", "[panda-event][tcp][ssl][socks]") {
    AsyncTest test(2000, {});
    in_port_t port = find_free_port();
    TCPSP client = make_client(test.loop);
    TCPSP server = make_server(port, test.loop);
   
    iptr<Stream> session; 
    server->connection_event.add([&](Stream*, Stream* s, const CodeError* err) {
        REQUIRE_FALSE(err);
        session = s;
        session->read_start([&](Stream*, const string&, const CodeError* err){
            REQUIRE_FALSE(err);
            test.loop->stop();
        });
    });

    client->connect("localhost", panda::to_string(port));
    client->write("123");

    test.loop->run();
}
