#pragma once
#include <chrono>
#include <catch.hpp>
#include <panda/unievent.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/test/AsyncTest.h>

using namespace panda;
using namespace panda::unievent;
using namespace panda::unievent::test;

struct Variation {
    bool ssl;
    bool buf;
};

extern Variation variation;

//TCPSP make_basic_server (Loop* loop, const SockAddr& sa = SockAddr::Inet4("127.0.0.1", 0));
//TCPSP make_server       (Loop* loop, const SockAddr& sa = SockAddr::Inet4("127.0.0.1", 0));
//TCPSP make_client       (Loop* loop, bool cached_resolver = true);

//SSL_CTX* get_ssl_ctx ();

//TimerSP read (StreamSP stream, Stream::read_fn callback, uint64_t timeout = 1000);
