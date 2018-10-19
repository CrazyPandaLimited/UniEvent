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
    bool   ssl;
    int    socks;
    string socks_url;
    bool   buf;
};

extern Variation variation;

TCPSP make_basic_server (uint16_t port, Loop* loop);
TCPSP make_socks_server (uint16_t port, LoopSP loop);
TCPSP make_server       (uint16_t port, Loop* loop);
TCPSP make_client       (Loop* loop, bool cached_resolver = true);

SSL_CTX* get_ssl_ctx ();

TimerSP read (StreamSP stream, Stream::read_fn callback, uint64_t timeout = 1000);
