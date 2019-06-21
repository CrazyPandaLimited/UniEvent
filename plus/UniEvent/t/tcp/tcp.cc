#include "../lib/test.h"
#include <iostream>
using std::cout;
using std::endl;

TEST_CASE("sync connect error", "[tcp][v-ssl][v-buf]") {
    AsyncTest test(2000, {"error"});
    net::SockAddr::Inet4 sa("255.255.255.255", 0); // makes underlying backend connect end with error synchronously

    TcpSP client = make_client(test.loop);
    client->connect_event.add([&](const StreamSP&, const CodeError& err, const ConnectRequestSP&) {
        REQUIRE(err.code());

        SECTION("disconnect") {
            client->disconnect();
        }
        SECTION("just go") {}
    });

    client->connect(sa);

    client->write("123");
    client->disconnect();

    auto res = test.await(client->write_event, "error");
    auto err = std::get<1>(res);
    REQUIRE(err.code() == std::errc::operation_canceled);
}

TEST_CASE("write without connection", "[tcp][v-ssl]") {
    AsyncTest test(2000, 1);
    TcpSP client = make_client(test.loop);
    client->write("1");
    client->write_event.add([&](auto, auto& err, auto) {
        REQUIRE(err.code() == std::errc::not_connected);
        test.happens();
    });
    test.run();
}

TEST_CASE("write to closed socket", "[tcp][v-ssl][v-buf]") {
    AsyncTest test(2000, {"error"});
    TcpSP server = make_server(test.loop);
    auto sa = server->sockaddr();

    TcpSP client = make_client(test.loop);
    client->connect(sa);
    client->write("1");
    test.await(client->write_event);
    client->disconnect();

    SECTION ("write") {
        client->write("2");
        client->write_event.add([&](Stream*, const CodeError& err, WriteRequest*) {
            WARN(err.what());
            REQUIRE(err.code() == std::errc::not_connected);
            test.happens("error");
            test.loop->stop();
        });
    }
    SECTION ("shutdown") {
        client->shutdown();
        client->shutdown_event.add([&](Stream*, const CodeError& err, ShutdownRequest*) {
            REQUIRE(err.code() == std::errc::not_connected);
            test.happens("error");
            test.loop->stop();
        });
    }
    test.loop->run();
}

TEST_CASE("immediate disconnect", "[tcp][v-ssl][v-buf]") {
    AsyncTest test(5000, {});
    SockAddr sa1, sa2;
    sa1 = sa2 = test.get_refused_addr();
    TcpSP server1, server2;
    SECTION ("no server") {}
    SECTION ("first no server second with server") {
        server2 = make_server(test.loop);
        sa2 = server2->sockaddr();
    }
    SECTION ("with servers") {
        server1 = make_server(test.loop);
        sa1 = server1->sockaddr();
        server2 = make_server(test.loop);
        sa2 = server2->sockaddr();
    }

    TcpSP client = make_client(test.loop);
    string body;
    for (size_t i = 0; i < 100; ++i) body += "0123456789";
    size_t write_count = 0;
    client->connect_event.add([&](Stream*, const CodeError& err, ConnectRequest*) {
        if (!err) client->disconnect();

        client->connect_event.remove_all();
        client->connect(sa2);

        for (size_t i = 0; i < 1200; ++i) {
            write_count++;
            client->write(body);
        }
        client->shutdown();
        client->disconnect();
    });

    size_t callback_count = 0;
    client->write_event.add([&](Stream*, const CodeError&, WriteRequest*){
        callback_count++;
        if (callback_count == write_count) {
            test.loop->stop();
        }
    });

    client->connect(sa1);

    test.run();
    REQUIRE(write_count == callback_count);
}

TEST_CASE("immediate client reset", "[tcp][v-ssl]") {
    AsyncTest test(2000, {"error"});
    SockAddr sa = test.get_refused_addr();
    TcpSP server;
    SECTION ("no server") {}
    SECTION ("with server") {
        server = make_server(test.loop);
        sa = server->sockaddr();
    }
    SECTION ("with nossl server") {
        server = make_basic_server(test.loop);
        sa = server->sockaddr();
    }
    TcpSP client = make_client(test.loop);

    client->connect(sa);

    client->connect_event.add([&](Stream*, const CodeError& err, ConnectRequest*) {
        CHECK(err.code() == std::errc::operation_canceled);
        test.happens("error");
    });

    client->reset();

}

TEST_CASE("immediate client write reset", "[tcp][v-ssl][v-buf]") {
    AsyncTest test(2000, {"c", "w"});
    TcpSP server = make_server(test.loop);
    TcpSP client = make_client(test.loop);

    client->connect_event.add([&](Stream*, const CodeError& err, ConnectRequest*) {
        test.happens("c");
        REQUIRE_FALSE(err);
        client->reset();
        test.loop->stop();
    });

    client->connect(server->sockaddr());
    client->write("123");
    client->write_event.add([&](Stream*, const CodeError& err, WriteRequest*) {
        test.happens("w");
        CHECK(err.code() == std::errc::operation_canceled);
    });

    test.loop->run();
}

TEST_CASE("reset accepted connection", "[tcp][v-ssl]") {
    AsyncTest test(2000, {"a"});
    TcpSP server = make_server(test.loop);
    TcpSP client = make_client(test.loop);

    server->connection_event.add([&](Stream*, Stream* client, const CodeError& err) {
        test.happens("a");
        REQUIRE_FALSE(err);
        client->reset();
        test.loop->stop();
    });

    client->connect(server->sockaddr());

    test.loop->run();
}

TEST_CASE("try use server without certificate", "[tcp]") {
    TcpSP server = new Tcp();
    server->bind("localhost", 0);

    SECTION("use_ssl after listen") {
        server->listen(1);
        REQUIRE_THROWS(server->use_ssl());
    }
    SECTION("use_ssl before listen") {
        server->use_ssl();
        REQUIRE_THROWS(server->listen(1));
    }
}

TEST_CASE("server read", "[tcp][v-ssl][v-buf]") {
    AsyncTest test(2000, {"c", "r"});
    TcpSP client = make_client(test.loop);
    TcpSP server = make_server(test.loop);

    StreamSP session;
    server->connection_event.add([&](Stream*, Stream* s, const CodeError& err) {
        test.happens("c");
        REQUIRE_FALSE(err);
        session = s;
        session->read_event.add([&](Stream*, string& str, const CodeError& err){
            test.happens("r");
            REQUIRE_FALSE(err);
            REQUIRE(str == "123");
            test.loop->stop();
        });
    });

    client->connect(server->sockaddr());
    client->write("123");

    test.loop->run();
}

//TODO: this test should have been failing before fix, but it did not
//TODO: find a way to reproduce SRV-1273 from UniEvent
TEST_CASE("UniEvent SRV-1273", "[tcp][v-ssl]") {
    AsyncTest test(1000, {});
    SockAddr addr = test.get_refused_addr();
    std::vector<TcpSP> clients;
    size_t counter = 0;

    auto client_timer = unievent::Timer::start(30, [&](TimerSP) {
        if (++counter == 10) {
            test.loop->stop();
        }
        TcpSP client = new Tcp(test.loop);
        client->connect_event.add([](Stream* s, const CodeError& err, ConnectRequest*){
            REQUIRE(err);
            s->reset();
        });

        client->connect(addr.ip(), addr.port());
        for (size_t i = 0; i < 2; ++i) {
            client->write("123", ([](Stream* s, const CodeError& err, WriteRequest*){
                REQUIRE(err);
                s->reset();
            }));
        }
        clients.push_back(client);
    }, test.loop);

    test.loop->run();
    clients.clear();
    REQUIRE(counter == 10);
}

TEST_CASE("MEIACORE-734 ssl server backref", "[tcp]") {
    AsyncTest test(500, {"connect"});
    TcpSP server = make_ssl_server(test.loop);
    TcpSP sconn;

    server->connection_factory = [&]() {
        sconn = new Tcp(test.loop);
        return sconn;
    };

    server->connection_event.add([&](auto...) {
        FAIL("should not be called");
    });

    TcpSP client = new Tcp(test.loop);
    client->connect(server->sockaddr());
    test.await(client->connect_event, "connect");

    server = nullptr;
    test.loop->run_nowait();
    client->reset();
    client = nullptr;

    test.run();
}

TEST_CASE("MEIACORE-751 callback recursion", "[tcp]") {
    AsyncTest test(500, {});
    SockAddr addr = test.get_refused_addr();

    TcpSP client = new Tcp(test.loop);

    size_t counter = 0;
    client->connect_event.add([&](Stream*, const CodeError&, ConnectRequest*) {
        if (++counter < 10) {
            client->connect()->to(addr.ip(), addr.port())->run();
            client->write("123");
        } else {
            test.loop->stop();
        }
    });

    client->connect()->to(addr.ip(), addr.port())->run();
    client->write("123");

    test.loop->run();
    REQUIRE(counter == 10);
}

TEST_CASE("correct callback order", "[tcp]") {
    AsyncTest test(900, {"connect", "write"});
    TcpSP server = make_basic_server(test.loop);
    SockAddr addr = server->sockaddr();

    TcpSP client = new Tcp(test.loop);
    client->connect()->to(addr.ip(), addr.port())->on_connect([&](Stream*, const CodeError&, ConnectRequest*){
        test.happens("connect");
    })->run();
    client->write("123", [&](Stream*, const CodeError&, WriteRequest*) {
        test.happens("write");
    });
    client->reset();
}

TEST_CASE("canceling queued requests with filter", "[tcp]") {
    TcpSP h = new Tcp();
    h->use_ssl();
    h->connect("localhost", 12345);
    h->disconnect();
    h->connect("localhost", 12345);
    h->write("lalala");
    h->write("hahaha");
    h->shutdown();
    h->disconnect();
    h->reset();
}

TEST_CASE("bind *", "[tcp]") {
    TcpSP h = new Tcp();
    h->bind("*", 12345);
}

TEST_CASE("write burst", "[tcp]") {
    AsyncTest test(500, 5);
    auto p = make_p2p(test.loop);

    p.sconn->read_event.add([&](auto, auto& str, auto){
        test.happens();
        CHECK(str == "abcd");
    });

    p.client->write("a");
    p.client->write("b");
    p.client->write("c");
    p.client->write("d");
    p.client->write_event.add([&](auto...) {
        test.happens();
    });

    test.loop->run_once();
}

TEST_CASE("write queue size", "[tcp]") {
    AsyncTest test(500, {});
    SECTION("queued") {
        auto p = make_tcp_pair(test.loop);
        CHECK(p.client->write_queue_size() == 0);
        p.client->write("a");
        CHECK(p.client->write_queue_size() == 1);
        p.client->write("b", [&](auto...){ test.loop->stop(); });
        CHECK(p.client->write_queue_size() == 2);
        test.run();
        CHECK(p.client->write_queue_size() == 0);
    }
    SECTION("sync") {
        auto p = make_p2p(test.loop);
        CHECK(p.client->write_queue_size() == 0);
        p.client->write("a");
        CHECK(p.client->write_queue_size() == 0);
        p.client->write("b");
        CHECK(p.client->write_queue_size() == 0);
    }
    SECTION("reset") {
        auto p = make_tcp_pair(test.loop);
        p.client->write("12345");
        CHECK(p.client->write_queue_size() == 5);
        p.client->write("67890");
        CHECK(p.client->write_queue_size() == 10);
        p.client->reset();
        CHECK(p.client->write_queue_size() == 0);
    }
    SECTION("sync-async") {
        auto p = make_p2p(test.loop);
        p.client->send_buffer_size(100000000);
        CHECK(p.client->write_queue_size() == 0);
        p.client->write("abcd");
        CHECK(p.client->write_queue_size() == 0);
        string epta;
        for (int i = 0; i < 1000000; ++i) epta += "1234567890";
        p.client->write(epta);
        CHECK(p.client->write_queue_size() > 0);
    }
}
