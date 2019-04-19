#pragma once
#include <mutex>
#include <chrono>
#include <thread>
#include <catch.hpp>
#include <panda/unievent.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/test/AsyncTest.h>

using namespace panda;
using namespace panda::unievent;
using namespace panda::unievent::test;
using ms  = std::chrono::milliseconds;
using sec = std::chrono::seconds;
using panda::net::SockAddr;

constexpr std::chrono::milliseconds operator""_ms (unsigned long long val) { return std::chrono::milliseconds(val); }
constexpr std::chrono::seconds      operator""_s  (unsigned long long val) { return std::chrono::seconds(val); }

struct Variation {
    bool ssl;
    bool buf;
};

extern Variation variation;

struct TimeGuard {
    std::thread t;
    std::timed_mutex m;

    TimeGuard (const std::chrono::milliseconds& tmt) {
        m.lock();
        t = std::thread([=]{
            if (!m.try_lock_for(tmt)) throw std::logic_error("Test timeouted");
        });
    }

    ~TimeGuard () {
        m.unlock();
        t.join();
    }
};

template <class T>
void time_guard (const std::chrono::milliseconds& tmt, T fn) {
    TimeGuard a(tmt);
    fn();
}

TcpSP make_basic_server (Loop* loop, const SockAddr& sa = SockAddr::Inet4("127.0.0.1", 0));
TcpSP make_server       (Loop* loop, const SockAddr& sa = SockAddr::Inet4("127.0.0.1", 0));
TcpSP make_client       (Loop* loop);

//SSL_CTX* get_ssl_ctx ();

//TimerSP read (StreamSP stream, Stream::read_fn callback, uint64_t timeout = 1000);
