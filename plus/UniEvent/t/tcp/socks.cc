#include "../lib/test.h"

TEST_CASE("bad socks server", "[socks]") {
    AsyncTest test(2000, {});

    TCPSP proxy_server = new TCP(test.loop);
    proxy_server->bind("localhost", "0");
    proxy_server->listen(1);
    auto sa = proxy_server->get_sockaddr();

    TCPSP client = new TCP(test.loop);
    client->use_ssl();
    client->use_socks(sa.ip(), sa.port());
   
    proxy_server->connection_event.add([&](Stream*, Stream* s, const CodeError* err) {
        REQUIRE_FALSE(err);
        s->write("bad socks");
        test.loop->stop();
    });

    client->connect(test.get_refused_addr());
    client->write("123");

    test.loop->run();
}

TEST_CASE("socks chain", "[socks][v-ssl]") {
    AsyncTest test(2000, {"ping", "pong"});
    size_t proxies_count = 5;
    std::vector<TCPSP> proxies;
    for (size_t i = 0; i < proxies_count; ++i) proxies.push_back(make_socks_server(test.loop));

    TCPSP server = make_basic_server(test.loop);
    auto sa = server->get_sockaddr();
    if (variation.ssl) server->use_ssl(get_ssl_ctx());
    server->connection_event.add([&test](Stream*, Stream* connection, const CodeError* err) {
        REQUIRE_FALSE(err);
        test.happens("ping");
        connection->write("pong");
        connection->shutdown();
    });

    TCPSP client = new TCP(test.loop);
    if (variation.ssl) client->use_ssl();
    for (auto proxy : proxies) {
        auto sa = proxy->get_sockaddr();
        client->push_behind_filter(new socks::SocksFilter(client, new Socks(sa.ip(), sa.port())));
    }
    client->connect(sa.ip(), string::from_number(sa.port()));
    client->write("ping");
    read(client, [&test](Stream*, const string& buf, const CodeError* err){
        REQUIRE_FALSE(err);
        REQUIRE(buf == string("pong"));
        test.happens("pong");
    });
    
    client->eof_event.add([&test, &server](Stream*){
        test.loop->stop();
    });
    test.loop->run();
}
