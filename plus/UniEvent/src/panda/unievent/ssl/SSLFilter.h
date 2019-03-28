//#pragma once
//#include <queue>
//#include <openssl/ssl.h>
//#include "../StreamFilter.h"
//
//namespace panda { namespace unievent { namespace ssl {
//
//struct SSLFilter : StreamFilter, AllocatedObject<SSLFilter, true> {
//    enum class State { initial = 0, negotiating = 1, error = 2, terminal = 3 };
//
//    static constexpr double PRIORITY = 1;
//    static const     void*  TYPE;
//
//    SSLFilter (Stream* h, SSL_CTX* context);
//    SSLFilter (Stream* h, const SSL_METHOD* method = nullptr);
//
//    StreamFilterSP clone () const override { return new SSLFilter(handle, SSL_get_SSL_CTX(ssl)); };
//
//    void on_connection (StreamSP, const CodeError*) override;
//    void on_connect    (const CodeError*, ConnectRequest*) override;
//    void write         (WriteRequest*) override;
//    void on_write      (const CodeError* err, WriteRequest* req) override;
//    void on_read       (string&, const CodeError*) override;
//    void on_eof        () override;
//    void on_reinit     () override;
//    void on_shutdown   (const CodeError* err, ShutdownRequest* shutdown_request) override;
//    bool is_secure     () override;
//    bool is_client     () const { return profile == Profile::CLIENT; }
//
//    void reset ();
//
//    SSL* get_ssl () const { return ssl; }
//
//    virtual ~SSLFilter ();
//
//private:
//    SSLFilter (iptr<SSLFilter> parent_filter, Stream* h, SSL_CTX* context);
//
//    enum class Profile { UNKNOWN = 0, SERVER = 1, CLIENT = 2 };
//
//    SSL*            ssl;
//    BIO*            read_bio;
//    BIO*            write_bio;
//    ConnectRequest* connect_request;
//    State           state;
//    Profile         profile;
//
//    weak_iptr<SSLFilter> parent_filter;
//
//    void init                 (SSL_CTX* context);
//    void start_ssl_connection (Profile);
//    int  negotiate            ();
//    void negotiation_finished (const CodeError* err = nullptr);
//    int  read_ssl_buffer      (string& decbuf, int pending);
//
//    static void on_negotiate_write (Stream*, const CodeError*, WriteRequest*);
//    static void on_regular_write   (Stream*, const CodeError*, WriteRequest*);
//};
//
//using SSLFilterSP = iptr<SSLFilter>;
//
//}}}
