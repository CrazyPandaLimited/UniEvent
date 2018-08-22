#pragma once
/*
#include <cstdint>

#include <panda/unievent/Debug.h>
#include <panda/unievent/Error.h>
#include <panda/string.h>
#include <panda/uri/socks.h>

namespace panda { namespace unievent {

using panda::uri::URI;
using panda::string;

struct Socks : public virtual Refcnt {
    ~Socks() {
        _EDEBUG("dtor");
    }

    Socks(const string& host, uint16_t port, const string& login = "", const string& passw = "", bool socks_resolve = true)
            : host(host)
            , port(port)
            , login(login)
            , passw(passw)
            , socks_resolve(socks_resolve) {
        _EDEBUG("ctor resolve through proxy: %s", socks_resolve ? "yes" : "no");
    }

    Socks(const URI& uri = URI(), bool socks_resolve = true) try : socks_resolve(socks_resolve) {
        _EDEBUG("ctor %.*s, resolve through proxy: %s", (int)uri.to_string().length(), uri.to_string().data(), socks_resolve ? "yes" : "no");

        if (uri.host()) {
            URI::socks socks_uri(uri);
            host  = socks_uri.host();
            port  = socks_uri.port();
            login = socks_uri.user();
            passw = socks_uri.password();
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
    } catch (...) {
        throw Error(string("Can't initialize Socks, url: ") + uri.to_string());
    }

    bool configured() const { return (bool)host; }

    bool loginpassw() const { return (bool)login; }

    string   host;
    uint16_t port;
    string   login;
    string   passw;
    bool     socks_resolve;
};

using SocksSP = iptr<Socks>;

}} // namespace panda::event
*/
