#include "lib/test.h"

TEST_CASE("bad socks server", "[panda-event][tcp][ssl][socks]") {
    AsyncTest test(2000, {});
    uint16_t port = find_free_port();
    uint16_t proxy_port = find_free_port();
    TCPSP client = new TCP(test.loop);
    client->use_ssl();
    client->use_socks("localhost", proxy_port);

    TCPSP proxy_server = new TCP(test.loop);
    proxy_server->bind("localhost", panda::to_string(proxy_port));
    proxy_server->listen(1);
   
    iptr<Stream> session; 
    proxy_server->connection_event.add([&](Stream*, Stream* s, const CodeError* err) {
        REQUIRE_FALSE(err);
        s->write("bad socks");
        test.loop->stop();
    });

    client->connect("localhost", panda::to_string(port));
    client->write("123");

    test.loop->run();
}

TEST_CASE("socks chain", "[panda-event][tcp][ssl][socks]") {
    AsyncTest test(2000, {"ping", "pong"});
    uint16_t port = find_free_port();
    struct Proxy { TCPSP server; uint16_t port; };
    size_t proxies_count = 5;
    std::vector<Proxy> proxies;
    std::generate_n(std::back_inserter(proxies), proxies_count, [&test]() {
        uint16_t port = find_free_port();
        return Proxy{make_socks_server(port, test.loop), port};
    });

    TCPSP server = make_basic_server(port, test.loop);
    if (variation.ssl) server->use_ssl(get_ssl_ctx());
    server->connection_event.add([&test](Stream*, Stream* connection, const CodeError* err) {
        REQUIRE_FALSE(err);
        test.happens("ping");
        connection->write("pong");
        connection->shutdown();
    });

    TCPSP client = new TCP(test.loop);
    if (variation.ssl) client->use_ssl();
    for(auto&& proxy : proxies) {
        client->push_behind_filter(new socks::SocksFilter(client, new Socks("localhost", proxy.port)));
    }
    client->connect("localhost", panda::to_string(port));
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
