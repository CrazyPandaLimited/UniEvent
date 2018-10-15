#pragma once
#include <cstdint>
#include <panda/unievent/Debug.h>
#include <panda/unievent/Error.h>
#include <panda/lib/memory.h>
#include <panda/string.h>
#include <panda/uri/socks.h>

namespace panda { namespace unievent {

struct Socks : virtual Refcnt {
    using URI = panda::uri::URI;

    ~Socks() {
        _EDTOR();
    }

    Socks(const string& host, uint16_t port, const string& login = "", const string& passw = "", bool socks_resolve = true)
            : host(host)
            , port(port)
            , login(login)
            , passw(passw)
            , socks_resolve(socks_resolve) {
        _ECTOR();
        _EDEBUG("resolve through proxy: %s", socks_resolve ? "yes" : "no");
    }

    Socks(const string& uri, bool socks_resolve = true) : Socks(URI::socks(uri), socks_resolve) {}

    Socks(const URI::socks& uri, bool socks_resolve = true) : socks_resolve(socks_resolve) {
        _ECTOR();
        _EDEBUG("%.*s, resolve through proxy: %s", (int)uri.to_string().length(), uri.to_string().data(), socks_resolve ? "yes" : "no");

        if (uri.host()) {
            host  = uri.host();
            port  = uri.port();
            login = uri.user();
            passw = uri.password();
        }

        if (login.length() > 0xFF) {
            throw Error("Bad login length");
        }

        if (passw.length() > 0xFF) {
            throw Error("Bad password length");
        }

        if (host)
            _EDEBUG("Socks: host:%.*s port:%u login:%.*s passw:%.*s, socks_resolve:%s", (int)host.length(), host.data(), port, (int)login.length(),
                    login.data(), (int)passw.length(), passw.data(), socks_resolve ? "yes" : "no");
    }

    bool configured() const { return !host.empty(); }

    bool loginpassw() const { return !login.empty(); }

    string   host;
    uint16_t port;
    string   login;
    string   passw;
    bool     socks_resolve;
};

using SocksSP = iptr<Socks>;

}} // namespace panda::event
