#include "AsyncTest.h"
#include <uv.h> // for getaddrinfo
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <panda/log.h>

namespace panda { namespace unievent { namespace test {

using panda::net::SockAddr;

SockAddr AsyncTest::get_refused_addr () {
    static SockAddr ret;
    while (!ret) {
        auto sock = socket(AF_INET, SOCK_STREAM, 0);   if (sock == -1) throw std::runtime_error("should not happen1");
        SockAddr sa = SockAddr::Inet4("127.0.0.1", 0);
        auto err = bind(sock, sa.get(), sa.length());  if (err == -1) throw std::runtime_error("should not happen2");
        socklen_t sz = sizeof(sa);
        err = getsockname(sock, sa.get(), &sz);        if (err == -1 || !sa) throw std::runtime_error("should not happen3");

        auto usock = socket(AF_INET, SOCK_DGRAM, 0);   if (usock == -1) throw std::runtime_error("should not happen4");
        err = bind(usock, sa.get(), sa.length());
        if (!err) {
            ret = sa;
            break;
        }

        err = close(sock);  if (err == -1) throw std::runtime_error("should not happen5");
        err = close(usock); if (err == -1) throw std::runtime_error("should not happen6");
    }
    return ret;
}

SockAddr AsyncTest::get_blackhole_addr () {
    addrinfo* res;
    int syserr = getaddrinfo("google.com", "81", NULL, &res);
    if (syserr) throw std::system_error(std::make_error_code(((std::errc)syserr)));
    return res->ai_addr;
}

AsyncTest::AsyncTest(uint64_t timeout, const std::vector<string>& expected)
    : loop(new Loop())
    , expected(expected)
    , timer(create_timeout(timeout))
{}

AsyncTest::~AsyncTest() noexcept(false) {
    if (!happened_as_expected() && !std::uncaught_exception()) {
        throw Error("Test exits in bad state", *this);
    }
}

void AsyncTest::run        () { loop->run(); }
void AsyncTest::run_once   () { loop->run_once(); }
void AsyncTest::run_nowait () { loop->run_nowait(); }

void AsyncTest::happens(string event) {
    if (event) {
        happened.push_back(event);
    }
}

std::string AsyncTest::generate_report() {
    std::stringstream out;

    for (size_t i = 0; i < std::max(expected.size(), happened.size()); ++i) {
        if (i >= expected.size()) {
            out << "\t\"" << happened[i] << "\" was not expected" << std::endl;
            continue;
        }
        if (i >= happened.size()) {
            out << "\t\"" << expected[i] << "\" has not happened" << std::endl;
            continue;
        }
        if (happened[i] != expected[i]) {
            out << "\t" << "wrong event " << happened[i] << ", " << expected[i] << " expected at pos " << i << std::endl;
            break;
        }
    }
    if (happened_as_expected()) {
        out << "OK" << std::endl;
    } else {
        out << "Expected: ";
        for (auto& e : expected) {
            out << "\"" << e << "\",";
        }
        out << std::endl << "Happened: ";
        for (auto& h : happened) {
            out << "\"" << h << "\",";
        }
        out << std::endl;
    }
    return out.str();
}

bool AsyncTest::happened_as_expected() {
    if (happened.size() != expected.size()) {
        return false;
    }
    for (size_t i = 0; i < happened.size(); ++i) {
        if (happened[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

sp<Timer> AsyncTest::create_timeout(uint64_t timeout) {
    auto ret = timer_once(timeout, loop, [&]() {
        throw Error("AsyncTest timeout", *this);
    });
    ret->weak(true);
    return ret;
}

AsyncTest::Error::Error(std::string msg, AsyncTest& test)
    : std::runtime_error(msg + "\nAsyncTest report:\n" + test.generate_report())
{}

}}}
