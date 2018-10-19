#pragma once

#include <queue>

#include <openssl/ssl.h>

#include "../StreamFilter.h"

namespace panda { namespace unievent { namespace ssl {

class SSLFilter : public StreamFilter, public AllocatedObject<SSLFilter, true> {
public:
    static const char* TYPE;

    enum class State { initial = 0, negotiating = 1, error = 2, terminal = 3 };

    SSLFilter (Stream* h, SSL_CTX* context);
    SSLFilter (Stream* h, const SSL_METHOD* method = nullptr);
    
    StreamFilterSP clone() const override { return StreamFilterSP(new SSLFilter(handle, SSL_get_SSL_CTX(ssl))); };

    void on_connection (StreamSP, const CodeError*) override;
    void on_connect    (const CodeError*, ConnectRequest*) override;
    void write         (WriteRequest*) override;
    void on_write      (const CodeError* err, WriteRequest* req) override;
    void on_read       (string&, const CodeError*) override;
    void on_eof        () override;
    void on_reinit     () override;
    void on_shutdown   (const CodeError* err, ShutdownRequest* shutdown_request) override; 
    bool is_secure     () override;

    void reset ();

    SSL* get_ssl () const { return ssl; }

    virtual ~SSLFilter ();

    bool is_client() const { return profile == Profile::CLIENT; }

private:
    SSLFilter (SSLFilter* parent_filter, Stream* h, SSL_CTX* context);

    enum class Profile { UNKNOWN = 0, SERVER = 1, CLIENT = 2 };

    SSL*            ssl;
    BIO*            read_bio;
    BIO*            write_bio;
    ConnectRequest* connect_request;
    SSLFilter*      parent_filter;
    State           state;
    Profile         profile;

    void init                 (SSL_CTX* context);
    void start_ssl_connection (Profile);
    int  negotiate            ();
    void negotiation_finished (const CodeError* err = nullptr);
    int read_ssl_buffer(string& decbuf, int pending);

    static bool openSSL_inited;
    static bool init_openSSL_lib ();
    static void on_negotiate_write (Stream*, const CodeError*, WriteRequest*);
    static void on_regular_write   (Stream*, const CodeError*, WriteRequest*);
};

}}}
