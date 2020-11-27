#include "../lib/test.h"

#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/engine.h>


static SslContext get_client_context(string name) {
    auto ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) throw Error(make_ssl_error_code(SSL_ERROR_SSL));

    auto r = SslContext::attach(ctx);

    string path("tests/cert/");
    string ca = path + "/ca.pem";
    string cert = path + "/" + name + ".pem";
    string key = path + "/" + name + ".key";
    int err;

    err = SSL_CTX_load_verify_locations(ctx, ca.c_str(), nullptr);
    assert(err);

    err = SSL_CTX_use_certificate_file(ctx, cert.c_str(), SSL_FILETYPE_PEM);
    assert(err);

    err = SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM);
    assert(err);

    SSL_CTX_check_private_key(ctx);
    assert(err);

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(ctx, 4);

    return r;
}

static SslContext get_server_context(string ca_name) {
    auto ctx = SSL_CTX_new(SSLv23_server_method());

    auto r = SslContext::attach(ctx);

    string path("tests/cert");
    string cert = path + "/" + ca_name + ".pem";
    string key = path + "/" + ca_name + ".key";
    int err;

    err = SSL_CTX_use_certificate_file(ctx, cert.c_str(), SSL_FILETYPE_PEM);
    assert(err);

    err = SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM);
    assert(err);

    err = SSL_CTX_check_private_key(ctx);
    assert(err);

    err = SSL_CTX_load_verify_locations(ctx, cert.c_str(), nullptr);
    assert(err);

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(ctx, 4);
    return r;
}


TEST_CASE("client custom certificate", "[ssl]") {
    AsyncTest test(10000, {"c", "r"});
    variation.ssl = true;

    TcpSP server = new Tcp(test.loop);
    auto server_cert = get_server_context("ca");
    auto server_sa = SockAddr::Inet4("127.0.0.1", 0);
    server->bind(server_sa);
    server->use_ssl(server_cert);
    server->listen(10000);

    StreamSP session;

    TcpSP client = new Tcp(test.loop, AF_INET);
    auto client_cert = get_client_context("01-alice");
    client->use_ssl(client_cert);

    server->connection_event.add([&](auto, auto s, auto& err) {
        test.happens("c");
        REQUIRE_FALSE(err);
        session = s;
        session->read_event.add([&](auto, string& str, auto& err){
            test.happens("r");
            REQUIRE_FALSE(err);
            REQUIRE(str == "123");
            test.loop->stop();
        });
    });

    client->connect(server->sockaddr().value());
    client->write("123");
    test.loop->run();
}

TEST_CASE("default client w/o certificate", "[ssl]") {
    AsyncTest test(2000, 0);
    variation.ssl = true;

    TcpSP server = new Tcp(test.loop);
    auto server_cert = get_server_context("ca");
    auto server_sa = SockAddr::Inet4("127.0.0.1", 0);
    server->bind(server_sa);
    server->use_ssl(server_cert);
    server->listen(10000);

    StreamSP session;

    TcpSP client = make_client(test.loop);

    server->connection_event.add([&](auto, auto, auto& err) {
        REQUIRE(err);
        REQUIRE(err & errc::ssl_error);
        test.loop->stop();
    });

    client->connect(server->sockaddr().value());
    client->write("123");
    test.loop->run();
}

TEST_CASE("server with different CA", "[ssl]") {
    AsyncTest test(2000, 0);
    variation.ssl = true;

    TcpSP server = new Tcp(test.loop);
    auto server_cert = get_server_context("ca2");
    auto server_sa = SockAddr::Inet4("127.0.0.1", 0);
    server->bind(server_sa);
    server->use_ssl(server_cert);
    server->listen(10000);

    StreamSP session;

    TcpSP client = new Tcp(test.loop, AF_INET);
    auto client_cert = get_client_context("01-alice");
    client->use_ssl(client_cert);

    server->connection_event.add([&](auto, auto&, auto& err) {
        REQUIRE(err);
        test.loop->stop();
    });

    client->connect(server->sockaddr().value());
    client->write("123");
    test.loop->run();
}

TEST_CASE("can't add filter when active", "[ssl]") {
    variation.ssl = false;
    AsyncTest test(1000);
    TcpSP client = make_client(test.loop);
    TcpSP server = make_server(test.loop);

    client->connect(server->sockaddr().value());
    CHECK_THROWS( client->use_ssl() );
    variation.ssl = true;
}
