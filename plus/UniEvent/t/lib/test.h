#pragma once
#include <chrono>
#include <catch.hpp>
#include <panda/unievent.h>
#include <panda/unievent/Debug.h>
#include <panda/unievent/test/AsyncTest.h>

using namespace panda;
using namespace panda::unievent;
using namespace panda::unievent::test;

// Uncomment to run tests                                                                                                                                
#define TEST_ASYNC 1 
#define TEST_CONNECT_TIMEOUT 1
#define TEST_RESOLVER 1
#define TEST_TIMER 1

extern bool TEST_SSL;
extern bool TEST_SOCKS;
extern bool TEST_BUF;

TCPSP make_basic_server (in_port_t port, Loop* loop);
TCPSP make_socks_server (in_port_t port, LoopSP loop);
TCPSP make_server       (in_port_t port, Loop* loop);
TCPSP make_client       (Loop* loop, bool cached_resolver = true);

SSL_CTX* get_ssl_ctx ();

TimerSP read (StreamSP stream, Stream::read_fn callback, uint64_t timeout = 1000);
