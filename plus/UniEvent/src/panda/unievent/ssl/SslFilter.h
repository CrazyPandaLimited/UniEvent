#pragma once
#include "../StreamFilter.h"
#include <openssl/ssl.h>
#include <panda/lib/memory.h>

namespace panda { namespace unievent { namespace ssl {

struct SslFilter;
using SslFilterSP = iptr<SslFilter>;

struct SslFilter : StreamFilter, panda::lib::AllocatedObject<SslFilter> {
    static constexpr double PRIORITY = 1;
    static const     void*  TYPE;

    SslFilter (Stream* h, SSL_CTX* context) : SslFilter(h, context, nullptr) {}
    SslFilter (Stream* h, const SSL_METHOD* method = nullptr);

    virtual ~SslFilter ();

    SSL* get_ssl () const { return ssl; }

    void listen () override;
    void handle_connection (const StreamSP&, const CodeError&) override;
    void handle_connect    (const CodeError&, const ConnectRequestSP&) override;
    void write             (const WriteRequestSP&) override;
    void handle_write      (const CodeError&, const WriteRequestSP&) override;
    void handle_read       (string&, const CodeError&) override;
    void handle_eof        () override;

    void reset () override;

private:
    enum class State   { initial = 0, negotiating = 1, error = 2, terminal = 3 };
    enum class Profile { UNKNOWN = 0, SERVER = 1, CLIENT = 2 };

    SslFilter (Stream* h, SSL_CTX* context, const SslFilterSP& server_filter);

    SSL*             ssl;
    BIO*             read_bio;
    BIO*             write_bio;
    ConnectRequestSP connect_request;
    State            state;
    Profile          profile;
    SslFilter*       server_filter;

    void init                 (SSL_CTX*);
    void start_ssl_connection (Profile);
    int  negotiate            ();
    void negotiation_finished (const CodeError& = CodeError());
};

}}}
