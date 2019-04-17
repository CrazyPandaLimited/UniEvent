#include "../lib/test.h"

TEST_CASE("sync connect error", "[tcp][v-ssl][v-buf]") {
    AsyncTest test(2000, {"error"});
    auto sa = test.get_refused_addr();

    TcpSP client = make_client(test.loop);
    client->connect_event.add([&](const StreamSP&, const CodeError* err, const ConnectRequestSP&) {
        REQUIRE(err);
        //_pex_(client)->type = UV_TCP;

        SECTION("disconnect") {
            client->disconnect();
        }
        SECTION("just go") {}
    });

    client->connect(sa.ip(), sa.port());
    //_pex_(client)->type = UV_HANDLE; // make uv_handle invalid for sync error after resolving

    client->write("123");
    client->disconnect();

    auto res = test.await(client->write_event, "error");
    auto err = std::get<1>(res);
    REQUIRE(err.code() == std::errc::operation_canceled);
}

//TEST_CASE("write to closed socket", "[tcp][v-ssl][v-buf]") {
//    AsyncTest test(2000, {"error"});
//    TCPSP server = make_server(test.loop);
//    auto sa = server->get_sockaddr();
//
//    TCPSP client = make_client(test.loop);
//    client->connect(sa);
//    client->write("1");
//    test.await(client->write_event);
//    client->disconnect();
//
//    if (false) { //TODO we need test params here
//        TimerSP t = Timer::once(10, [](Timer*){}, test.loop);
//        test.await(t->timer_event);
//    }
//
//    SECTION ("write") {
//        client->write("2");
//        client->write_event.add([&](Stream*, const CodeError* err, WriteRequest*) {
//            REQUIRE(err);
//            REQUIRE(err->code() == ERRNO_EBADF);
//            test.happens("error");
//            test.loop->stop();
//        });
//    }
//    SECTION ("shutdown") {
//        client->shutdown();
//        client->shutdown_event.add([&](Stream*, const CodeError* err, ShutdownRequest*) {
//            REQUIRE(err);
//            REQUIRE(err->code() == ERRNO_ENOTCONN);
//            test.happens("error");
//            test.loop->stop();
//        });
//    }
//    test.loop->run();
//}
//
//TEST_CASE("immediate disconnect", "[tcp][v-ssl][v-buf]") {
//    AsyncTest test(5000, {});
//    SockAddr sa1, sa2;
//    sa1 = sa2 = test.get_refused_addr();
//    TCPSP server1, server2;
//    SECTION ("no server") {}
//    SECTION ("first no server second with server") {
//        server2 = make_server(test.loop);
//        sa2 = server2->get_sockaddr();
//    }
//    SECTION ("with servers") {
//        server1 = make_server(test.loop);
//        sa1 = server1->get_sockaddr();
//        server2 = make_server(test.loop);
//        sa2 = server2->get_sockaddr();
//    }
//
//    TCPSP client = make_client(test.loop);
//    string body;
//    for (size_t i = 0; i < 100; ++i)  {
//        body += "0123456789";
//    }
//    size_t write_count = 0;
//    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
//        if (!err) client->disconnect();
//
//        client->connect_event.remove_all();
//        client->connect(sa2);
//
//        for (size_t i = 0; i < 1200; ++i) {
//            write_count++;
//            client->write(body);
//        }
//        client->shutdown();
//        client->disconnect();
//    });
//
//    size_t callback_count = 0;
//    client->write_event.add([&](Stream*, const CodeError*, WriteRequest*){
//        callback_count++;
//        if (callback_count == write_count) {
//            test.loop->stop();
//        }
//    });
//
//    client->connect(sa1);
//
//    test.run();
//    REQUIRE(write_count == callback_count);
//}
//
//TEST_CASE("immediate client reset", "[tcp][v-ssl]") {
//    AsyncTest test(2000, {"error"});
//    SockAddr sa = test.get_refused_addr();
//    TCPSP server;
//    SECTION ("no server") {}
//    SECTION ("with server") {
//        server = make_server(test.loop);
//        sa = server->get_sockaddr();
//    }
//    SECTION ("with nossl server") {
//        server = make_basic_server(test.loop);
//        sa = server->get_sockaddr();
//    }
//    TCPSP client = make_client(test.loop);
//
//    client->connect(sa);
//    client->reset();
//
//    auto res = test.await(client->connect_event, "error");
//    auto err = std::get<1>(res);
//    REQUIRE(err.code() == ERRNO_ECANCELED);
//}
//
//TEST_CASE("immediate client write reset", "[tcp][v-ssl][v-buf]") {
//    AsyncTest test(2000, {});
//    TCPSP server = make_server(test.loop);
//    TCPSP client = make_client(test.loop);
//
//    client->connect_event.add([&](Stream*, const CodeError* err, ConnectRequest*) {
//        REQUIRE_FALSE(err);
//        client->reset();
//        test.loop->stop();
//    });
//
//    client->connect(server->get_sockaddr());
//    client->write("123");
//
//    test.loop->run();
//}
//
//TEST_CASE("reset accepted connection", "[tcp][v-ssl]") {
//    AsyncTest test(2000, {});
//    TCPSP server = make_server(test.loop);
//    TCPSP client = make_client(test.loop);
//
//    server->connection_event.add([&](Stream*, Stream* client, const CodeError* err) {
//        REQUIRE_FALSE(err);
//        client->reset();
//        test.loop->stop();
//    });
//
//    client->connect(server->get_sockaddr());
//
//    test.loop->run();
//}
//
//TEST_CASE("try use server without certificate 1", "[tcp][v-ssl]") {
//    AsyncTest test(2000, {});
//    TCPSP server = new TCP(test.loop);
//    server->bind("localhost", 0);
//    server->listen(1);
//    REQUIRE_THROWS(server->use_ssl());
//}
//
//TEST_CASE("try use server without certificate 2", "[tcp][v-ssl]") {
//    AsyncTest test(2000, {});
//    TCPSP server = new TCP(test.loop);
//    server->bind("localhost", 0);
//    server->use_ssl();
//    REQUIRE_THROWS(server->listen(1));
//}
//
//TEST_CASE("server read", "[tcp][v-ssl][v-buf]") {
//    AsyncTest test(2000, {});
//    TCPSP client = make_client(test.loop);
//    TCPSP server = make_server(test.loop);
//
//    StreamSP session;
//    server->connection_event.add([&](Stream*, Stream* s, const CodeError* err) {
//        REQUIRE_FALSE(err);
//        session = s;
//        session->read_start([&](Stream*, const string&, const CodeError* err){
//            REQUIRE_FALSE(err);
//            test.loop->stop();
//        });
//    });
//
//    client->connect(server->get_sockaddr());
//    client->write("123");
//
//    test.loop->run();
//}
////TODO: this test should have been failing before fix, but it did not
////TODO: find a way to reproduce SRV-1273 from UniEvent
//TEST_CASE("UniEvent SRV-1273", "[tcp][v-ssl]") {
//    AsyncTest test(1000, {"client", "client", "client", "client", "client", "client", "client", "client", "client", "client"});
//    SockAddr addr = test.get_refused_addr();
//    std::vector<TCPSP> clients;
//    size_t counter = 0;
//
//    auto client_timer = unievent::Timer::start(30, [&](TimerSP) {
//        if (++counter == 10) {
//            test.loop->stop();
//        }
//        test.happens("client");
//        TCPSP client = new TCP(test.loop);
//        client->connect_event.add([](Stream* s, const CodeError* err, ConnectRequest*){
//            REQUIRE(err);
//            s->reset();
//        });
//
//        client->connect(addr.ip(), addr.port());
//        for (size_t i = 0; i < 2; ++i) {
//            client->write("123", ([](Stream* s, const CodeError* err, WriteRequest*){
//                REQUIRE(err);
//                s->reset();
//            }));
//        }
//        clients.push_back(client);
//    }, test.loop);
//
//    test.loop->run();
//    clients.clear();
//    REQUIRE(counter == 10);
//}
//
//TEST_CASE("MEIACORE-734 ssl server backref", "[tcp]") {
//    AsyncTest test(500, {"connect", "done"});
//    TCPSP server = make_basic_server(test.loop);
//    server->use_ssl(get_ssl_ctx());
//
//    TCPSP sconn;
//
//    server->connection_factory = [&]() {
//        sconn = new TCP(test.loop);
//        return sconn;
//    };
//
//    bool destroyed = false;
//
//    server->connection_event.add([&](Stream*, StreamSP, const CodeError*) {
//        REQUIRE_FALSE(destroyed);
//    });
//
//    TCPSP client = new TCP(test.loop);
//    client->connect(server->get_sockaddr());
//    test.await(client->connect_event, "connect");
//
//    server.reset();
//    test.loop->run_nowait();
//    destroyed = true;
//    client->reset();
//    client.reset();
//
//    test.wait(30);
//    test.happens("done");
//}
//
//TEST_CASE("SRV-1273 #1", "[tcp]") {
//    AsyncTest test(500, {});
//    SockAddr addr = test.get_refused_addr();
//
//    TCPSP client = new TCP(test.loop);
//    client->connect().to(addr);
//    client->reset();
//    client->connect().to(addr);
//    client->reset();
//    REQUIRE(client->async_locked());
//}
//
//TEST_CASE("MEIACORE-751 callback recursion", "[tcp]") {
//    AsyncTest test(500, {});
//    SockAddr addr = test.get_refused_addr();
//
//    TCPSP client = new TCP(test.loop);
//
//
//    size_t counter = 0;
//    client->connect_event.add([&](Stream*, const CodeError*, ConnectRequest*) {
//        if (++counter < 10) {
//            client->connect().to(addr.ip(), addr.port());
//            client->write("123");
//        } else {
//            test.loop->stop();
//        }
//    });
//
//    client->connect().to(addr.ip(), addr.port());
//    client->write("123");
//
//    test.loop->run();
//    REQUIRE(counter == 10);
//}
//
////TEST_CASE("wrong callback order", "[tcp]") {
////    //Behavior is wrong, callback order is opposite, but it always worked this way
////    AsyncTest test(900, {"write", "connect"});
////    TCPSP server = make_basic_server(test.loop);
////    SockAddr addr = server->get_sockaddr();
//
////    TCPSP client = new TCP(test.loop);
////    client->connect().to(addr.ip(), addr.port()).callback([&](Stream*, const CodeError*, ConnectRequest*){
////        test.happens("connect");
////    });
////    client->write("123", [&](Stream*, const CodeError*, WriteRequest*) {
////        test.happens("write");
////    });
////    client->reset();
////    test.await(client->connect_event);
////}
//
////TEST_CASE("SRV-1273 #2", "[tcp]") {
////    AsyncTest test(90000, {});
////    TCPSP server = make_basic_server(test.loop);
////    SockAddr addr = server->get_sockaddr();
////    std::vector<TCPSP> clients;
//
////    auto connect = [&](TCPSP client) {
////        client->connect().to(addr.ip(), addr.port());
//////        client->connect().to(addr);
////    };
//
////    PrepareSP tick = new Prepare(test.loop);
////    tick->prepare_event.add([](PrepareSP) {
////        panda_log_debug("tick");
////    });
////    tick->start();
//
////    TimerSP creator = Timer::start(10, [&](TimerSP) {
////        TCPSP client = new TCP(test.loop);
////        client->read_start();
////        connect(client);
////        client->write("123", [&](Stream* s, const CodeError* err, WriteRequest*) {
////            TCPSP client = dyn_cast<TCP*>(s);
////            //connect(client);
////            client->connect().to(addr.ip(), addr.port()+1).timeout(5);
////            client->write("123");
////        });
////        clients.push_back(client);
////    },test.loop);
//
////    TimerSP timer = Timer::start(200, [&](TimerSP) {
////        for (TCPSP c : clients) {
////            c->reset();
////            connect(c);
////            c->write("123");
////            c->disconnect();
////        }
////    },test.loop);
//
////    test.loop->run();
////    clients.clear();
////}
