#pragma once
#include <queue>
#include <openssl/ssl.h>
#include "../StreamFilter.h"

namespace panda { namespace unievent { namespace ssl {

class SSLFilter : public StreamFilter, public AllocatedObject<SSLFilter, true> {
public:
    static const char* TYPE;

    SSLFilter (Stream* h, SSL_CTX* context);
    SSLFilter (Stream* h, const SSL_METHOD* method = nullptr);

    void accept     (Stream*) override;
    void on_connect (const CodeError*, ConnectRequest*) override;
    void write      (WriteRequest*) override;
    void on_write   (const CodeError* err, WriteRequest* req) override;
    void on_read    (string&, const CodeError*) override;
    void reset      () override;
    bool is_secure  () override;

    SSL* get_ssl () const { return ssl; }

    virtual ~SSLFilter ();

private:
    enum class Profile {
        SERVER = 0,
        CLIENT = 1
    };

    SSL*            ssl;
    BIO*            read_bio;
    BIO*            write_bio;
    Profile         profile;
    ConnectRequest* connect_request;
    bool            started;

    void init                 (SSL_CTX* context);
    void start_ssl_connection (Profile);
    int  negotiate            ();
    void negotiation_finished (const CodeError* err = nullptr);

    static bool openSSL_inited;
    static bool init_openSSL_lib ();
    static void on_negotiate_write (Stream*, const CodeError*, WriteRequest*);
    static void on_regular_write   (Stream*, const CodeError*, WriteRequest*);
};

}}}
